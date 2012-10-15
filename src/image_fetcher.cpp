#include "image_fetcher.hpp"
#include <iostream>
#include <glibmm/fileutils.h>

namespace Horizon {
	std::shared_ptr<ImageFetcher> ImageFetcher::get(FETCH_TYPE type) {
		static std::shared_ptr<ImageFetcher> singleton_4chan;
		static std::shared_ptr<ImageFetcher> singleton_catalog;
		
		if (!singleton_4chan) {
			singleton_4chan = std::shared_ptr<ImageFetcher>(new ImageFetcher());
		}

		if (!singleton_catalog) {
			singleton_catalog = std::shared_ptr<ImageFetcher>(new ImageFetcher());
		}
		
		switch (type) {
		case FOURCHAN:
			return singleton_4chan;
		case CATALOG:
			return singleton_catalog;
		default:
			return singleton_4chan;
		}
	}
	
	const Glib::RefPtr<Gdk::Pixbuf> ImageFetcher::get_thumb(const std::string &hash) const {
		Glib::Mutex::Lock lock(thumbs_mutex);
		
		auto iter = thumbs.find(hash);
		if ( iter != thumbs.end() ) {
			return iter->second;
		} else {
			g_error("ImageFetcher::get_thumb called for an image not yet downloaded");
			return Glib::RefPtr<Gdk::Pixbuf>();
		}
	}

	const Glib::RefPtr<Gdk::Pixbuf> ImageFetcher::get_image(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		
		auto iter = images.find(hash);
		if ( iter != images.end() ) {
			return iter->second;
		} else {
			g_error("ImageFetcher::get_image called for an image not yet downloaded");
			return Glib::RefPtr<Gdk::Pixbuf>();
		}
	}

	const Glib::RefPtr<Gdk::PixbufAnimation> ImageFetcher::get_animation(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		
		auto iter = animations.find(hash);
		if ( iter != animations.end() ) {
			return iter->second;
		} else {
			g_error("ImageFetcher::get_animation called for an image not yet downloaded");
			return Glib::RefPtr<Gdk::PixbufAnimation>();
		}
	}

	bool ImageFetcher::has_thumb(const std::string &hash) const {
		Glib::Mutex::Lock lock(thumbs_mutex);
		return thumbs.count(hash) > 0;
	}

	bool ImageFetcher::has_image(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		return images.count(hash) > 0;
	}

	bool ImageFetcher::has_animation(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		return animations.count(hash) > 0;
	}

	static void stream_destroy_notify(gpointer data) {
		g_free(data);
	}

	std::size_t horizon_curl_writeback(char *ptr, size_t nmemb, size_t size, void *userdata) {
		std::size_t wrote = 0;
		std::shared_ptr<Request> request = *static_cast<std::shared_ptr<Request>*>(userdata);
		std::string hash = request->hash;
		ImageFetcher* ifetcher = static_cast<ImageFetcher*>(request->image_fetcher);
		Glib::RefPtr< Gdk::PixbufLoader > loader;
		Glib::RefPtr< Gio::MemoryInputStream > istream = request->istream;
		
		loader.reset();

		if ( request->is_thumb ) {
			Glib::Mutex::Lock lock(ifetcher->thumbs_mutex);
			auto iter = ifetcher->thumb_streams.find(hash);
			if ( G_LIKELY(iter != ifetcher->thumb_streams.end())) {
				loader = iter->second;
			} else {
			g_critical("horizon_curl_writeback called with unknown hash %s.",
			           hash.c_str());
			return 0;
			}
		} else {
			Glib::Mutex::Lock lock(ifetcher->images_mutex);
			auto iter = ifetcher->image_streams.find(hash);
			if ( G_LIKELY(iter != ifetcher->image_streams.end())) {
				loader = iter->second;
			} else {
			g_critical("horizon_curl_writeback called with unknown hash %s.",
			           hash.c_str());
			return 0;
			}
		}

		if (G_LIKELY(loader) && istream) {
			try {
				loader->write(reinterpret_cast<guint8*>(ptr),
				              static_cast<gsize>(size*nmemb));
				wrote = size * nmemb;
				gpointer buf = g_memdup(ptr, size * nmemb);
				istream->add_data(buf, size * nmemb, stream_destroy_notify);
			} catch (Gdk::PixbufError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			} catch (Glib::FileError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			}
		} else {
			g_critical("Got an invalid loader or stream");
			return 0;
		}

		return wrote;
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::start_new_download() {
		std::list< std::pair<std::shared_ptr<CurlEasy>,
		                     std::shared_ptr<Request>* > > work_list;
		{
			Glib::Mutex::Lock lock(curl_data_mutex);
			while ( !curl_queue.empty() ) {
				if (request_queue.empty()) {
					break;
				} else {
					std::shared_ptr<CurlEasy> easy = curl_queue.front();
					curl_queue.pop();
					std::shared_ptr<Request> *request = new std::shared_ptr<Request>();
					*request = request_queue.front();
					request_queue.pop();
					work_list.push_back({easy, request});
				}
			}
		}

		for (auto pair : work_list) {
			std::shared_ptr<CurlEasy> easy = pair.first;
			std::shared_ptr<Request> *request = pair.second;
				
			if ((*request)->is_thumb) {
				Glib::Mutex::Lock lock(thumbs_mutex);
				if (thumb_streams.count((*request)->hash) > 0) {
					request->reset();
				}
			} else {
				Glib::Mutex::Lock lock(images_mutex);
				if (image_streams.count((*request)->hash) > 0) {
					request->reset();
				}
			}
		
			if (*request) {
				const std::string hash = (*request)->hash;
				const std::string url = (*request)->url;
				const bool is_thumb = (*request)->is_thumb;
				const std::string ext = (*request)->ext;
				Glib::RefPtr<Gdk::PixbufLoader> loader;
				loader.reset();
				bool loader_error = false;

				easy->reset();
				easy->set_write_function(horizon_curl_writeback, static_cast<void*>(request));
				easy->set_url(url);
				easy->set_private(static_cast<void*>(request));
				easy->set_fail_on_error();
				easy->set_error_buffer(curl_error_buffer);
				easy->set_connect_timeout(3);
				easy->set_no_signal();
			
				try {
					// Supports "jpeg" "gif" "png"
					loader = Gdk::PixbufLoader::create();
				} catch ( Gdk::PixbufError e ) {
					std::cerr << "Pixbuf error starting download: " 
					          << e.what() << std::endl;
					loader_error = true;
				}

				if (!loader_error && loader) {
					if (is_thumb) {
						{
							Glib::Mutex::Lock lock(thumbs_mutex);
							thumb_streams.insert({hash, loader});
						}
						curl_multi->add_handle(easy);
					} else {
						{
							Glib::Mutex::Lock lock(images_mutex);
							image_streams.insert({hash, loader});
						}
						curl_multi->add_handle(easy);
					}
				} else {
					std::cerr << "Download aborted for url " << url << std::endl;
					Glib::Mutex::Lock lock(curl_data_mutex);
					curl_queue.push(easy);
					queue_w.send();
				}
			} else {
				Glib::Mutex::Lock lock(curl_data_mutex);
				curl_queue.push(easy);
				queue_w.send();
			}
		} // for work_list
	}

	/*
	 * Called from Glib main thread
	 */
	void ImageFetcher::download_thumb(const Glib::RefPtr<Post> &post) {
		//const std::string &hash, const std::string &url) {
		std::shared_ptr<Request> req(new Request());
		req->hash = post->get_hash();
		req->url = post->get_thumb_url();
		req->is_thumb = true;
		req->ext = ".jpg";
		req->image_fetcher = this;
		req->istream = Gio::MemoryInputStream::create();
		req->post = post;

		if (image_cache->has_thumb(post)) {
			auto functor = std::bind(&ImageFetcher::on_cache_result,
			                         this,
			                         std::placeholders::_1,
			                         req);
			image_cache->get_thumb_async(post, functor);
		} else {
			Glib::Mutex::Lock lock(curl_data_mutex);
			request_queue.push(req);

			queue_w.send();
		}
	}

	/*
	 * Called from Glib main thread
	 */
	void ImageFetcher::download_image(const Glib::RefPtr<Post> &post) {
		//const std::string &hash, const std::string &url, const std::string &ext) {
		std::shared_ptr<Request> req(new Request());
		req->hash = post->get_hash();
		req->url = post->get_image_url();
		req->is_thumb = false;
		req->ext = post->get_image_ext();
		req->image_fetcher = this;
		req->istream = Gio::MemoryInputStream::create();
		req->post = post;

		if (image_cache->has_image(post)) {
			auto functor = std::bind(&ImageFetcher::on_cache_result,
			                         this,
			                         std::placeholders::_1,
			                         req);
			image_cache->get_image_async(post, functor);
		} else {
			Glib::Mutex::Lock lock(curl_data_mutex);
			request_queue.push(req);

			queue_w.send();
		}
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
		Socket_Info* info = static_cast<Socket_Info*>(socketp);
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
			ev_tstamp SEC_PER_MS = 0.001;
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
		info->w = std::shared_ptr<ev::io>(new ev::io(ev_loop));
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
				// We SHOULD NOT call info->channel->close(). This closes curl's
				// cached connection to the server.
				info->w->stop();
				socket_info_cache_.push_back(info);
				active_socket_infos_.remove(info);
				active_sockets_.remove(s);
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
	                                         bool is_404) {
		const bool isThumb = request->is_thumb;
		Glib::RefPtr<Gdk::PixbufLoader> loader;
		loader.reset();

		if ( isThumb ) {
			// This is a thumbnail
			Glib::Mutex::Lock lock(thumbs_mutex);
			auto iter = thumb_streams.find(request->hash);
			if (iter != thumb_streams.end()) {
				loader = iter->second;
			} else {
				g_warning("Couldn't find loader for given hash on thumbnail %s",
				          request->url.c_str());
			}
		} else {
			Glib::Mutex::Lock lock(images_mutex);
			auto iter = image_streams.find(request->hash);
			if ( iter != image_streams.end() ) {
				loader = iter->second;
			} else {
				g_warning("Couldn't find loader for given hash on image %s",
				        request->url.c_str());
			}
		}

		if (G_LIKELY(loader)) {
			try {
				loader->close();
			} catch (Gdk::PixbufError e) {
				// This is expected since this was a failed download
			} catch (Glib::FileError e) {
			}
		}

		if ( isThumb ) {
			Glib::Mutex::Lock lock(thumbs_mutex);
			auto iter = thumb_streams.find(request->hash);
			if (iter != thumb_streams.end()) {
				thumb_streams.erase(iter);
			}
		} else {
			Glib::Mutex::Lock lock(images_mutex);
			auto iter = image_streams.find(request->hash);
			if ( iter != image_streams.end() ) {
				image_streams.erase(iter);
			}
		}
		
		if (is_404) {
			GError *error = nullptr;
			const char* resource_404 = "/com/talisein/fourchan/native/gtk/404-Anonymous-2.png";
			GdkPixbuf *cpixbuf_404 = gdk_pixbuf_new_from_resource(resource_404,
			                                                      &error);
			if (!error) {
				auto pixbuf_404 = Glib::wrap(cpixbuf_404);
				{
					Glib::Mutex::Lock lock(pixbuf_mutex);
					pixbuf_map.insert({request, pixbuf_404});
				}
				signal_pixbuf_ready();
			} else {
				g_warning("Unable to create 404 image from resource: %s",
				          error->message);
			}
		}
	}

	/*
	 * Called on ev_thread
	 */
	void ImageFetcher::create_pixmap(std::shared_ptr<Request> request) {
		const bool isThumb = request->is_thumb;
		const std::string hash = request->hash;
		const bool isGif = request->ext.rfind("gif") != std::string::npos;
		Glib::RefPtr<Gdk::PixbufLoader> loader;
		loader.reset();
		bool close_error = false;
		bool get_error = false;

		if ( isThumb ) {
			// This is a thumbnail
			Glib::Mutex::Lock lock(thumbs_mutex);
			auto iter = thumb_streams.find(request->hash);
			if (iter != thumb_streams.end()) {
				loader = iter->second;
			} else {
				g_warning("Couldn't find loader for given hash on thumbnail %s",
				          request->url.c_str());
				return;
			}
		} else {
			Glib::Mutex::Lock lock(images_mutex);
			auto iter = image_streams.find(request->hash);
			if ( iter != image_streams.end() ) {
				loader = iter->second;
			} else {
				g_warning("Couldn't find loader for given hash on image %s",
				          request->url.c_str());
				return;
			}
		}

		if ( G_LIKELY( loader ) ) {
			try {
				loader->close();
			} catch (Gdk::PixbufError e) {
				std::cerr << "Pixbuf Error creating pixmap for url " << request->url
				          << ": " << e.what() << std::endl;
				close_error = true;
			} catch (Glib::FileError e) {
				std::cerr << "File error creating pixmap for url " << request->url
				          << ": " << e.what() << std::endl;
				close_error = true;
				// TODO: Determine which errors we can recover
				// from by trying again
			}
		} else {
			g_error( "Loader doesn't have a valid pointer." );
			return;
		}

		if (!close_error && loader) {
			if (isGif) {
				Glib::RefPtr<Gdk::PixbufAnimation> pixbuf_animation;
				try {
					pixbuf_animation = loader->get_animation();
				} catch (Gdk::PixbufError e) {
					std::cerr << "Unexpected Pixbuf error fetching "
					          << "from loader for url " 
					          << request->url << ": "
					          << e.what() << std::endl;
					get_error = true;
				}

				if (!get_error && pixbuf_animation) {
					{
						Glib::Mutex::Lock lock(pixbuf_mutex);
						pixbuf_animation_map.insert({request, pixbuf_animation});
					}
					signal_pixbuf_ready();
				}
			} else {
				Glib::RefPtr<Gdk::Pixbuf> pixbuf;
				try {
					pixbuf = loader->get_pixbuf();
				} catch (Gdk::PixbufError e) {
					std::cerr << "Unexpected Pixbuf error fetching "
					          << "from loader for url " 
					          << request->url << ": "
					          << e.what() << std::endl;
					get_error = true;
				}
				
				if (!get_error && pixbuf) {
					{
						Glib::Mutex::Lock lock(pixbuf_mutex);
						pixbuf_map.insert({request, pixbuf});
					}
					signal_pixbuf_ready();
				}
			}
		}

		if ( !close_error && !get_error ) {
			// This was good data, so save to disk.
			if (isThumb) {
				image_cache->write_thumb_async(request->post,
				                               request->istream);
			} else {
				image_cache->write_image_async(request->post,
				                               request->istream);
			}
		} else {
			// Bad data? Close the stream.
			request->istream->close();
		}

		if ( isThumb ) {
			Glib::Mutex::Lock lock(thumbs_mutex);
			auto iter = thumb_streams.find(request->hash);
			if (iter != thumb_streams.end()) {
				thumb_streams.erase(iter);
			}
		} else {
			Glib::Mutex::Lock lock(images_mutex);
			auto iter = image_streams.find(request->hash);
			if ( iter != image_streams.end() ) {
				image_streams.erase(iter);
			}
		}

	}

	/* 
	 * Called from ImageCache thread
	 */
	void ImageFetcher::on_cache_result(const Glib::RefPtr<Gdk::PixbufLoader>& loader,
	                                   std::shared_ptr<Request> request) {
		bool load_error = false;

		if (loader) {
			if ( request->ext.find(".gif") != request->ext.npos ) {
				Glib::RefPtr<Gdk::PixbufAnimation> ani_pixbuf;
				try {
					ani_pixbuf = loader->get_animation();
				} catch (Gdk::PixbufError e) {
					load_error = true;
					g_warning("Error loading from pixbuf: %s", e.what().c_str());
				}

				if (ani_pixbuf && !load_error) {
					Glib::Mutex::Lock lock(pixbuf_mutex);
					pixbuf_animation_map.insert({request, ani_pixbuf});
					signal_pixbuf_ready();
				}
			} else {
				Glib::RefPtr<Gdk::Pixbuf> pixbuf;
				try {
					pixbuf = loader->get_pixbuf();
				} catch (Gdk::PixbufError e) {
					load_error = true;
					g_warning("Error loading from pixbuf: %s", e.what().c_str());
				}
				
				if (pixbuf && !load_error) {
					Glib::Mutex::Lock lock(pixbuf_mutex);
					pixbuf_map.insert({request, pixbuf});
					signal_pixbuf_ready();
				}
			}
		}
	}

	/*
	 * Called on the GLib Main Thread
	 */
	void ImageFetcher::on_pixbuf_ready() {
		Glib::Mutex::Lock lock(pixbuf_mutex);
		if (pixbuf_map.size() > 0) {
			auto iter = pixbuf_map.begin();
			if (iter->first->is_thumb) {
				{
					Glib::Mutex::Lock lock(thumbs_mutex);
					thumbs.insert({iter->first->hash, iter->second});
				}
				signal_thumb_ready(iter->first->hash);
			} else {
				{
					Glib::Mutex::Lock lock(images_mutex);
					images.insert({iter->first->hash, iter->second});
				}
				signal_image_ready(iter->first->hash);
			}
			pixbuf_map.erase(iter);
		} else

		if (pixbuf_animation_map.size() > 0) {
			auto iter = pixbuf_animation_map.begin();
			{
				Glib::Mutex::Lock lock(images_mutex);
				animations.insert({iter->first->hash, iter->second});
			}
			signal_image_ready(iter->first->hash);
			pixbuf_animation_map.erase(iter);
		}
	}


	/* 
	 * Called on ev_thread
	 *
	 * Lock already held here
	 */
	void ImageFetcher::curl_check_info() {
		int msgs_left;
		CURL* curl;
		CURLcode res;
		CURLMsg* msg;
		bool download_error = false;
		bool download_error_404 = false;
		std::shared_ptr<Request> *request_p;
		std::shared_ptr<CurlEasy> easy;
		do { 
			msg = curl_multi->info_read(&msgs_left);
			if (msg) {
				switch(msg->msg) {
				case CURLMSG_DONE:
					curl = msg->easy_handle;
					res = msg->data.result;
					curl_easy_getinfo(curl, CURLINFO_PRIVATE, reinterpret_cast<void**>(&request_p));
					if ( G_UNLIKELY(res != CURLE_OK) ) {
						download_error = true;
						long code = 0;
						curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
						if (res  == CURLE_HTTP_RETURNED_ERROR &&
						    code == 404) {
							download_error_404 = true;
						} else {
							std::cerr << "Error: Downloading " << (*request_p)->url
							          << " : " << curl_error_buffer << std::endl;
						}
					}

					easy = curl_multi->remove_handle(curl);
					/*
					mcode = curl_multi_remove_handle(curlm, curl);
					if (G_UNLIKELY(mcode != CURLM_OK)) {
						g_error("While removing curl handle from multi: %s",
						        curl_multi_strerror(mcode));
					}
					*/
					if (G_LIKELY(!download_error)) {
						create_pixmap(*request_p);
					} else {
						cleanup_failed_pixmap(*request_p, download_error_404);
					}

					delete request_p;
					
					{
						Glib::Mutex::Lock lock(curl_data_mutex);
						curl_queue.push(easy);
					}
					queue_w.send();
					break;
				case CURLMSG_NONE:
					g_warning("Warning: Unexpected MSG_NONE from curl info read.");
					break;
				case CURLMSG_LAST:
					g_warning("Warning: Unexpected MSG_LAST from curl info read.");
					break;
				default:
					g_warning("Unexpected message %d from curl info read.", msg->msg);
				}
			}
		} while(msg != NULL && msgs_left > 0);
	}

	void ImageFetcher::on_kill_loop_w(ev::async &, int) {
		kill_loop_w.stop();
		queue_w.stop();
	}

	void ImageFetcher::on_timeout_w(ev::timer &, int) {
		curl_timeout_expired_cb();
	}

	void ImageFetcher::on_queue_w(ev::async &, int) {
		start_new_download();
	}

	ImageFetcher::ImageFetcher() :
		image_cache(ImageCache::get()),
		curl_error_buffer(g_new0(char, CURL_ERROR_SIZE)),
		curl_multi(CurlMulti::create()),
		running_handles(0),
		ev_loop(ev::AUTO),
		kill_loop_w(ev_loop),
		queue_w(ev_loop),
		timeout_w(ev_loop)
	{
		Glib::Mutex::Lock lock(curl_data_mutex);
		signal_pixbuf_ready.connect( sigc::mem_fun(*this, &ImageFetcher::on_pixbuf_ready) );

		curl_multi->set_socket_function(&curl_socket_cb, this);
		curl_multi->set_timer_function(&curl_timer_cb, this);

		for (int i = 0; i < 3; i++) {
			auto easy = CurlEasy::create();
			curl_queue.push(easy);
		}

		queue_w.set<ImageFetcher, &ImageFetcher::on_queue_w> (this);
		queue_w.start();

		kill_loop_w.set<ImageFetcher, &ImageFetcher::on_kill_loop_w> (this);
		kill_loop_w.start();

		timeout_w.set<ImageFetcher, &ImageFetcher::on_timeout_w> (this);

		ev_thread = Glib::Threads::Thread::create( sigc::bind(sigc::mem_fun(ev_loop, &ev::dynamic_loop::run),0) );
		if (!ev_thread)
			g_error("Couldn't spin up ImageFetcher thread");
	}

	ImageFetcher::~ImageFetcher() {
		kill_loop_w.send();
		ev_thread->join();

		g_free(curl_error_buffer);
	}
}
