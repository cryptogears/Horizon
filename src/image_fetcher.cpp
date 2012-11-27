#include "image_fetcher.hpp"
#include <iostream>
#include <atomic>
#include <glibmm/fileutils.h>
#include "utils.hpp"

#include "horizon_curl.cpp"

namespace Horizon {
	std::size_t ImageFetcher::curl_writeback(const std::string &str_buf,
	                                         std::shared_ptr<Request> request) {
		std::size_t wrote = 0;
		std::string hash = request->hash;
		Glib::RefPtr< Gdk::PixbufLoader > loader = request->loader;
		Glib::RefPtr< Gio::MemoryInputStream > istream = request->istream;
		
		if (G_LIKELY( loader && istream )) {
			try {
				loader->write(reinterpret_cast<const guint8*>(str_buf.c_str()),
				              static_cast<gsize>(str_buf.size()));
				wrote = str_buf.size();
				gpointer buf = g_memdup(str_buf.c_str(), str_buf.size());
				istream->add_data(buf, str_buf.size(), &g_free);
			} catch (Gdk::PixbufError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			} catch (Glib::FileError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			}
		} else {
			g_critical("Got an invalid loader or stream");
		}

		return wrote;
	}


	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::start_new_download() {
		std::vector< std::pair<std::shared_ptr<CurlEasy<std::shared_ptr<Request> > >,
		                       std::shared_ptr<Request> > > work_list;
		{
			Glib::Threads::RWLock::WriterLock wlock(request_queue_rwlock);
			Glib::Threads::Mutex::Lock lock(curl_data_mutex);
			while ( (!curl_queue.empty()) && (!new_request_queue.empty()) ) {
					std::shared_ptr<CurlEasy<std::shared_ptr<Request> > > easy = curl_queue.front();
					curl_queue.pop();
					std::pop_heap(new_request_queue.begin(),
					              new_request_queue.end(),
					              request_comparitor);
					std::shared_ptr<Request> request = new_request_queue.back();
					new_request_queue.pop_back();
					work_list.push_back(std::make_pair(easy, request));
			}
		}

		for (auto pair : work_list) {
			std::shared_ptr<CurlEasy<std::shared_ptr<Request> > > easy = pair.first;
			std::shared_ptr<Request> request = pair.second;
				
			if (request) {
				const std::string url  = request->url;

				easy->reset();
				auto write_functor = std::bind(&ImageFetcher::curl_writeback,
				                               this,
				                               std::placeholders::_1,
				                               request);
				easy->set_write_function(write_functor);
				easy->set_url(url);
				easy->set_private(request);
				easy->set_fail_on_error();
				easy->set_error_buffer(curl_error_buffer);
				easy->set_connect_timeout(3);
				easy->set_no_signal();
			
				try {
					// Supports "jpeg" "gif" "png"
					request->loader = Gdk::PixbufLoader::create();
					if (request->area_prepared_functor) {
						auto area_prepared_slot = sigc::bind(sigc::mem_fun(*this, &ImageFetcher::on_area_prepared),
						                                     request);
						request->area_prepared_connection = request->loader->signal_area_prepared().connect(area_prepared_slot);
					}
					auto area_updated_slot = sigc::bind(sigc::mem_fun(*this, &ImageFetcher::on_area_updated),
					                                    request);
					request->area_updated_connection = request->loader->signal_area_updated().connect(area_updated_slot);

					curl_multi->add_handle(easy);
				} catch ( Gdk::PixbufError e ) {
					std::cerr << "Download aborted for url " << url << std::endl;
					Glib::Threads::Mutex::Lock lock(curl_data_mutex);
					curl_queue.push(easy);
					queue_w.send();
				}

			} else {
				Glib::Threads::Mutex::Lock lock(curl_data_mutex);
				curl_queue.push(easy);
				queue_w.send();
			}
		} // for work_list
	}
	
	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::on_area_prepared(std::shared_ptr<Request> request) {
		if (request->area_prepared_functor) {
			auto pixbuf = request->loader->get_pixbuf();
			std::function<void ()> f = std::bind(request->area_prepared_functor, pixbuf);
			{
				Glib::Threads::Mutex::Lock lock(pixbuf_update_queue_mutex);
				auto pair = std::make_pair(request->canceller, f);
				pixbuf_update_queue.push_back(pair);
			}

			signal_pixbuf_updated();
		}

		request->area_prepared_connection.disconnect();
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::on_area_updated(int x, int y, int width, int height,
	                                   std::shared_ptr<Request> request) {
		if (request->area_updated_functor) {
			std::function<void ()> f = std::bind(request->area_updated_functor, x, y, width, height);
			{
				auto pair = std::make_pair(request->canceller, f);
				pixbuf_update_queue.push_back(pair);
			}
			auto is_connected = pixbuf_updated_idle_is_connected.exchange(true);
			if (!is_connected)
				signal_pixbuf_updated();
		}
	}

	/*
	 * Called on Glib main thread
	 */
	void ImageFetcher::signal_pixbuf_updated_dispatched() {
		if (!pixbuf_updated_idle.connected()) {
			pixbuf_updated_idle = Glib::signal_idle().connect(sigc::mem_fun(*this, &ImageFetcher::on_pixbuf_updated));
		}
	}

	/*
	 * Called on Glib::Main thread idle
	 */
	bool ImageFetcher::on_pixbuf_updated() {
		bool ret = true;
		{
			Glib::Threads::Mutex::Lock lock(pixbuf_update_queue_mutex);
			if (pixbuf_update_queue.size() > 0) {
				auto pair = pixbuf_update_queue.front();
				auto canceller = pair.first;
				auto functor = pair.second;
				canceller->involke_if_not_cancelled(functor);
				pixbuf_update_queue.pop_front();
			}

			if ( pixbuf_update_queue.size() == 0 ) {
				pixbuf_updated_idle_is_connected = false;
				pixbuf_updated_idle.disconnect();
				ret = false;
			} else {
				ret = true;
			}
		}

		return ret;
	}

	static std::string get_request_key(const Glib::RefPtr<Post> &post, bool get_thumb) {
		std::string hash;
		if (get_thumb)
			hash = "T_";
		else
			hash = "I_";
		hash.append(post->get_hash());

		return hash;
	}
	
	static std::shared_ptr<Request> create_request(const Glib::RefPtr<Post> &post,
	                                               bool is_thumb,
	                                               std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared_cb,
	                                               std::function<void (int, int, int, int)> area_updated_cb,
	                                               std::shared_ptr<Canceller> canceller) {
		static std::atomic<uint64_t> serial(0);
		std::shared_ptr<Request> req = std::make_shared<Request>();
		req->hash = post->get_hash();
		req->is_thumb = is_thumb;
		if (is_thumb) {
			req->url = post->get_thumb_url();
			req->ext = ".jpg";
		} else {
			req->url = post->get_image_url();
			req->ext = post->get_image_ext();
		}
		req->istream = Gio::MemoryInputStream::create();
		req->post = post;
		req->area_prepared_functor = area_prepared_cb;
		req->area_updated_functor = area_updated_cb;
		req->canceller = canceller;
		req->serial = serial++;
		return req;
	}

	/*
	 * Called from Glib main thread
	 */
	void ImageFetcher::download(const Glib::RefPtr<Post> &post,
	                            std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &loader)> callback,
	                            std::shared_ptr<Canceller> canceller,
	                            bool get_thumb,
	                            std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared_cb,
	                            std::function<void (int, int, int, int)> area_updated_cb) {
		if (!post) {
			g_warning("ImageFetcher::download() called with invalid post");
			return;
		}
		std::string request_key = get_request_key(post, get_thumb);

		bool is_pending = add_request_cb(request_key, callback);
		if ( !is_pending ) {
			bool in_cache = false;
			std::shared_ptr<Request> request = create_request(post, get_thumb,
			                                                  area_prepared_cb,
			                                                  area_updated_cb,
			                                                  canceller);
			auto cache_cb = std::bind(&ImageFetcher::on_cache_result,
			                          this,
			                          std::placeholders::_1,
			                          request);
			if (get_thumb) {
				if (image_cache->has_thumb(post)) {
					image_cache->get_thumb_async(post, cache_cb, this->canceller);
					in_cache = true;
				}
			} else {
				if (image_cache->has_image(post)) {
					image_cache->get_image_async(post, cache_cb, this->canceller);
					in_cache = true;
				}
			}

			if (!in_cache) {
				add_request(request);
			}
		}
	}

	/*
	 * Called from Glib main thread
	 */
	bool ImageFetcher::add_request_cb(const std::string &request_key,
	                                  std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &)> cb) {
		bool is_pending = false;
		Glib::Threads::RWLock::WriterLock lock(request_cb_rwlock);

		auto iter = request_cb_map.find(request_key);
		if ( iter != request_cb_map.end() )
			is_pending = true;
		
		request_cb_map.insert(iter, std::make_pair(request_key, cb));

		return is_pending;
	}

	/*
	 * Called from Glib main thread
	 */
	void ImageFetcher::add_request(const std::shared_ptr<Request> &request) {
		Glib::Threads::RWLock::WriterLock lock(request_queue_rwlock);
		
		new_request_queue.push_back(request);
		std::push_heap(new_request_queue.begin(),
		               new_request_queue.end(),
		               request_comparitor);
		queue_w.send();
	}

	/*
	 * Called on ev_thread
	 */
	int curl_socket_cb(CURL *easy,      /* easy handle */
	                   curl_socket_t s, /* socket */
	                   int action,      /* see values below */
	                   void *userp,     /* private callback pointer */
	                   void *socketp)   /* private socket pointer */
	{
		Socket_Info *info      = static_cast<Socket_Info*>(socketp);
		ImageFetcher *ifetcher = static_cast<ImageFetcher*>(userp);
		
		if (action == CURL_POLL_REMOVE) {
			ifetcher->curl_remsock(info, s);
		} else {
			if (!info) {
				ifetcher->curl_addsock(s, easy, action);
			} else {
				ifetcher->curl_setsock(info, s, easy, action);
			}
		}
		return 0;
	}

	/*
	 * Called on ev_thread
	 */
	int curl_timer_cb(CURLM *, long timeout_ms, void *userp) {
		ImageFetcher *ifetcher = static_cast<ImageFetcher*>(userp);

		if (ifetcher->timeout_w.is_active())
			ifetcher->timeout_w.stop();

		if (timeout_ms == -1) {
		} else if (timeout_ms == 0) {
			ifetcher->curl_timeout_expired_cb();
		} else {
			constexpr ev_tstamp SEC_PER_MS = 0.001;
			ev_tstamp timeout = static_cast<ev_tstamp>(timeout_ms) * SEC_PER_MS;
			ifetcher->timeout_w.set( timeout );
			ifetcher->timeout_w.start();
		}

		return 0;
	}

	/*
	 * Called on ev_thread
	 */
	bool ImageFetcher::curl_timeout_expired_cb() {
		int old_handles = running_handles;
		curl_multi->socket_action_timeout(&running_handles);

		if ( running_handles < old_handles)
			curl_check_info();
		return false;
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::curl_event_cb(ev::io &w, int) {
		curl_socket_t s = static_cast<curl_socket_t>(w.fd);
		int old_handles = running_handles;

		curl_multi->socket_action(s, 0, &running_handles);

		if ( running_handles < old_handles)
			curl_check_info();
		if (running_handles == 0) {
			if (timeout_w.is_active())
				timeout_w.stop();
		}
	}
	
	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::curl_setsock(Socket_Info* info, curl_socket_t s,
	                                CURL*, int action) {
		int condition = 0;

		if (action & CURL_POLL_IN)
			condition = ev::READ;
		if (action & CURL_POLL_OUT)
			condition = condition | ev::WRITE;
		if (action & CURL_POLL_INOUT)
			condition = condition | ev::WRITE | ev::READ;

		if (info->w)
			info->w.reset();
		info->w = std::make_shared<ev::io>(ev_loop);
		info->w->set<ImageFetcher, &ImageFetcher::curl_event_cb>(this);
		info->w->set(static_cast<int>(s), condition);
		info->w->start();
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::curl_addsock(curl_socket_t s, CURL *easy, int action) {
		if( G_LIKELY( std::count(active_sockets_.begin(),
		                         active_sockets_.end(), s) == 0 )) {
			active_sockets_.push_back(s);
		} else {
			g_critical("Error: curl_addsock got called twice for the same socket. "
			           "I don't know what to do!");
		}

		Socket_Info* info;
		if (socket_info_cache_.empty()) {
			info = new Socket_Info();
		} else {
			info = socket_info_cache_.back();
			socket_info_cache_.pop_back();
		}

		if ( G_LIKELY(std::count(active_socket_infos_.begin(),
		                         active_socket_infos_.end(), info) == 0 )) {
			active_socket_infos_.push_back(info);
		} else {
			g_critical("Error: Somehow the info we are assigning is already active!");
		}

		curl_setsock(info, s, easy, action);
		curl_multi->multi_assign(s, info);
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::curl_remsock(Socket_Info* info, curl_socket_t s) {
		if (G_UNLIKELY( !info ))
			return;

		if (G_LIKELY( std::count(active_sockets_.begin(),
		                         active_sockets_.end(), s) > 0 )) {
			if (G_LIKELY( std::count(active_socket_infos_.begin(),
			                         active_socket_infos_.end(), info) > 0 )) {
				info->w->stop();
				socket_info_cache_.push_back(info);
				active_socket_infos_.erase(std::remove(active_socket_infos_.begin(),
				                                       active_socket_infos_.end(),
				                                       info),
				                           active_socket_infos_.end());
				active_sockets_.erase(std::remove(active_sockets_.begin(),
				                                  active_sockets_.end(),
				                                  s),
				                      active_sockets_.end());
			} else {
				g_critical("curl_remsock called on inactive info.");
			}
		} else {
			g_warning("curl_remsock was called twice for the same socket.");
		}
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::cleanup_failed_pixmap(std::shared_ptr<Request> request,
	                                         bool) { /* is_404 */
		Glib::RefPtr<Gdk::PixbufLoader> loader = request->loader;

		if (G_LIKELY( loader )) {
			try {
				loader->close();
			} catch (Gdk::PixbufError e) {
				// This is expected since this was a failed download
			} catch (Glib::FileError e) {
			}
		}

		/*
		 * TODO: for !404 errors, put the requests in a cooldown queue
		 */
		request->area_updated_connection.disconnect();
		loader.reset();
		bind_loader_to_callbacks(request, loader);
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::create_pixmap(std::shared_ptr<Request> request) {
		const bool isThumb = request->is_thumb;
		Glib::RefPtr<Gdk::PixbufLoader> loader = request->loader;
		bool close_error = false;

		if ( G_LIKELY( loader ) ) {
			try {
				loader->close();
			} catch (Gdk::PixbufError e) {
				std::cerr << "Error: Closing PixbufLoader for url " << request->url
				          << ": " << e.what() << std::endl;
				close_error = true;
			} catch (Glib::FileError e) {
				std::cerr << "Error: Closing PixbufLoader for url " << request->url
				          << ": " << e.what() << std::endl;
				close_error = true;
				// TODO: Determine which errors we can recover
				// from by trying again
			}
		} else {
			g_error( "Loader doesn't have a valid pointer." );
			return;
		}

		request->area_updated_connection.disconnect();

		if (close_error) {
			loader.reset();
			request->istream->close();
		} else {
			if (isThumb) {
				image_cache->write_thumb_async(request->post,
				                               request->istream);
			} else {
				image_cache->write_image_async(request->post,
				                               request->istream);
			}
		}

		bind_loader_to_callbacks(request, loader);
	}

	/*
	 * Called on ImageCache thread or ev_thread
	 */
	bool ImageFetcher::bind_loader_to_callbacks(std::shared_ptr<Request> request,
	                                            const Glib::RefPtr<Gdk::PixbufLoader> &loader) {
		std::string request_key = get_request_key(request->post,
		                                          request->is_thumb);
		std::shared_ptr<Canceller> canceller = request->canceller;
		bool found_cb = false;
		Glib::Threads::RWLock::WriterLock lock(request_cb_rwlock);
		auto iter_pair = request_cb_map.equal_range(request_key);
		auto lower_bound = iter_pair.first;
		auto upper_bound = iter_pair.second;
		if (lower_bound != request_cb_map.end()) {
			found_cb = true;
			Glib::Threads::RWLock::WriterLock cb_queue_lock(cb_queue_rwlock);
			std::transform(lower_bound,
			               upper_bound,
			               std::back_inserter(cb_queue),
			               [&loader, &canceller](const std::pair<std::string,
			                         std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &)>
			                         > &pair) {
				               std::function<void ()> f = std::bind(pair.second, loader);
				               return std::make_pair(canceller, f);
			               });
			request_cb_map.erase(lower_bound, upper_bound);
			auto is_connected = cb_queue_is_connected.exchange(true);
			if (!is_connected) {
				signal_process_cb_queue();
			}
		}

		return found_cb;
	}

	/*
	 * Called on Glib Main Loop
	 */
	void ImageFetcher::signal_process_cb_queue_dispatched() {
		if (!cb_queue_idle.connected()) {
			cb_queue_idle =  Glib::signal_idle().connect(sigc::mem_fun(*this, &ImageFetcher::process_cb_queue),
			                                             Glib::PRIORITY_DEFAULT);
		}
	}

	/*
	 * Called on Glib MainLoop idle
	 */
	bool ImageFetcher::process_cb_queue() {
		bool ret = true;
		Glib::Threads::RWLock::WriterLock cb_queue_lock(cb_queue_rwlock);
		if ( !cb_queue.empty() ) {
			auto pair = cb_queue.front();
			auto canceller = pair.first;
			auto cb = pair.second;
			canceller->involke_if_not_cancelled(cb);
			cb_queue.pop_front();
		}

		if ( cb_queue.empty() ) {
			ret = false;
			cb_queue_is_connected = false;
			cb_queue_idle.disconnect();
		}

		return ret;
	}

	/* 
	 * Called from ImageCache thread
	 */
	void ImageFetcher::on_cache_result(const Glib::RefPtr<Gdk::PixbufLoader>& loader,
	                                   std::shared_ptr<Request> request) {
		if (loader) {
			bind_loader_to_callbacks(request, loader);
		} else {
			// There was an error from disk, so fallback to downloading.
			std::cerr << "Info: An on disk image was invalid or corrupt, so we are downloading it again." << std::endl;
			add_request(request);
		}

	}

	/* 
	 * Called on ev_thread
	 *
	 * Lock already held here
	 */
	void ImageFetcher::curl_check_info() {
		int msgs_left;
		CURLcode res;
		bool download_error = false;
		bool download_error_404 = false;
		std::shared_ptr<Request> request;
		std::shared_ptr<CurlEasy<std::shared_ptr<Request> > > easy;
		do { 
			auto pair = curl_multi->info_read(&msgs_left);
			easy = pair.first;
			res = pair.second;
			if (easy) {
				request = easy->get_private();

				if ( G_UNLIKELY(res != CURLE_OK) ) {
					download_error = true;
					long code = easy->get_response_code();
					if (res  == CURLE_HTTP_RETURNED_ERROR &&
					    code == 404) {
						download_error_404 = true;
					} else {
						std::cerr << "Error: Downloading " << request->url
						          << " : " << curl_error_buffer << std::endl;
					}
				}

				curl_multi->remove_handle(easy);

				if (G_LIKELY(!download_error)) {
					create_pixmap(request);
				} else {
					cleanup_failed_pixmap(request, download_error_404);
				}

				{
					Glib::Threads::Mutex::Lock lock(curl_data_mutex);
					curl_queue.push(easy);
				}
				queue_w.send();
			}
		} while(easy && msgs_left > 0);
	}

	void ImageFetcher::on_kill_loop_w(ev::async &, int) {
		kill_loop_w.stop();
		queue_w.stop();
		timeout_w.stop();
	}

	void ImageFetcher::on_timeout_w(ev::timer &, int) {
		curl_timeout_expired_cb();
	}

	void ImageFetcher::on_queue_w(ev::async &, int) {
		start_new_download();
	}

	ImageFetcher::ImageFetcher(const std::shared_ptr<ImageCache>& cache) :
		canceller(std::make_shared<Canceller>()),
		image_cache(cache),
		cb_queue_is_connected(false),
		pixbuf_updated_idle_is_connected(false),
		curl_error_buffer(g_new0(char, CURL_ERROR_SIZE)),
		curl_multi(CurlMulti<std::shared_ptr<Request> >::create()),
		running_handles(0),
		ev_thread(nullptr),
		ev_loop(ev::AUTO | ev::POLL),
		kill_loop_w(ev_loop),
		queue_w(ev_loop),
		timeout_w(ev_loop)
	{
		signal_pixbuf_updated.connect(sigc::mem_fun(*this, &ImageFetcher::signal_pixbuf_updated_dispatched));
		signal_process_cb_queue.connect(sigc::mem_fun(*this, &ImageFetcher::signal_process_cb_queue_dispatched));
		Glib::Threads::Mutex::Lock lock(curl_data_mutex);

		curl_multi->set_socket_function(&curl_socket_cb, this);
		curl_multi->set_timer_function(&curl_timer_cb, this);

		for (int i = 0; i < 6; i++) {
			auto easy = CurlEasy<std::shared_ptr<Request> >::create();
			curl_queue.push(easy);
		}

		queue_w.set<ImageFetcher, &ImageFetcher::on_queue_w> (this);
		queue_w.start();

		kill_loop_w.set<ImageFetcher, &ImageFetcher::on_kill_loop_w> (this);
		kill_loop_w.start();

		timeout_w.set<ImageFetcher, &ImageFetcher::on_timeout_w> (this);

		ev_loop.set_io_collect_interval(0.1);

		const sigc::slot<void> slot = sigc::bind(sigc::mem_fun(ev_loop, &ev::dynamic_loop::run),0);
		int trycount = 0;

		while ( ev_thread == nullptr && trycount++ < 10 ) {
			try {
				ev_thread = Horizon::create_named_thread("ImageFetcher",
				                                         slot);
			} catch ( Glib::Threads::ThreadError e) {
				if (e.code() != Glib::Threads::ThreadError::AGAIN) {
					g_error("Couldn't create ImageFetcher thread: %s",
					        e.what().c_str());
				}
			}
		}
		
		if (!ev_thread)
			g_error("Couldn't spin up ImageFetcher thread");
	}

	ImageFetcher::~ImageFetcher() {
		canceller->cancel();
		if (cb_queue_idle)
			cb_queue_idle.disconnect();
		if (pixbuf_updated_idle)
			pixbuf_updated_idle.disconnect();

		kill_loop_w.send();
		ev_thread->join();

		g_free(curl_error_buffer);
		for ( Socket_Info *info : socket_info_cache_ ) {
			delete info;
		}

		if ( active_socket_infos_.size() > 0 ) {
			g_warning("Curl has left active sockets laying around");
			for ( Socket_Info *info : active_socket_infos_ ) {
				delete info;
			}
		}
	}
}
