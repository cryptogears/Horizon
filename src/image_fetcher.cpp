#include "image_fetcher.hpp"
#include <iostream>
#include <glibmm/fileutils.h>

namespace Horizon {
	std::shared_ptr<ImageFetcher> ImageFetcher::get() {
		static std::shared_ptr<ImageFetcher> singleton;
		
		if (!singleton) {
			singleton = std::shared_ptr<ImageFetcher>(new ImageFetcher());
		}
		
		if (!singleton) {
			g_critical("Unable to create the image fetcher.");
		}
		
		return singleton;
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

	const bool ImageFetcher::has_thumb(const std::string &hash) const {
		Glib::Mutex::Lock lock(thumbs_mutex);
		return thumbs.count(hash) > 0;
	}

	const bool ImageFetcher::has_image(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		return images.count(hash) > 0;
	}

	const bool ImageFetcher::has_animation(const std::string &hash) const {
		Glib::Mutex::Lock lock(images_mutex);
		return animations.count(hash) > 0;
	}

	static void horizon_destroy_notify(gpointer ptr) {
		g_free(ptr);
	}

	std::size_t horizon_curl_writeback(char *ptr, size_t nmemb, size_t size, void *userdata) {
		std::size_t wrote = 0;
		std::shared_ptr<Request> request = *static_cast<std::shared_ptr<Request>*>(userdata);
		std::string hash = request->hash;
		std::shared_ptr<ImageFetcher> ifetcher = ImageFetcher::get();
		Glib::RefPtr< Gdk::PixbufLoader > loader;
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

		if (G_LIKELY(loader)) {
			try {
				loader->write(reinterpret_cast<guint8*>(ptr),
				              static_cast<gsize>(size*nmemb));
				wrote = size * nmemb;
			} catch (Gdk::PixbufError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			} catch (Glib::FileError e) {
				std::cerr << "Error writing pixbuf from network: " << e.what() << std::endl;
			}
		} else {
			g_critical("Got an invalid loader");
			return 0;
		}

		return wrote;
	}

	void ImageFetcher::start_new_download(CURL *curl) {
		std::shared_ptr<Request> *request = new std::shared_ptr<Request>();
		{
			Glib::Mutex::Lock lock(curl_data_mutex);
			while (!(*request)) {
				if (request_queue.empty()) {
					curl_queue.push(curl);
					return;
				} else {
					*request = request_queue.front();
					request_queue.pop();
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
				}
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

			curl_easy_reset(curl);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, horizon_curl_writeback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(request));
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_PRIVATE, static_cast<void*>(request));
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer);
			
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
					curl_multi_add_handle(curlm, curl);
				} else {
					{
						Glib::Mutex::Lock lock(images_mutex);
						image_streams.insert({hash, loader});
					}
					curl_multi_add_handle(curlm, curl);
				}
			} else {
					std::cerr << "Download aborted for url " << url << std::endl;
			} 
		}
	}

	void ImageFetcher::download_thumb(const std::string &hash, const std::string &url) {
		std::shared_ptr<Request> req(new Request());
		CURL* curl = nullptr;
		req->hash = hash;
		req->url = url;
		req->is_thumb = true;
		req->ext = ".jpg";

		{
			Glib::Mutex::Lock lock(curl_data_mutex);
			request_queue.push(req);

			if ( ! curl_queue.empty() ) {
				curl = curl_queue.front();
				curl_queue.pop();
			}
		}

		if (curl)
			start_new_download(curl);
	}

	void ImageFetcher::download_image(const std::string &hash, const std::string &url, const std::string &ext) {
		std::shared_ptr<Request> req(new Request());
		CURL* curl = nullptr;
		req->hash = hash;
		req->url = url;
		req->is_thumb = false;
		req->ext = ext;

		{
			Glib::Mutex::Lock lock(curl_data_mutex);
			request_queue.push(req);
			if ( !curl_queue.empty() ) {
				curl = curl_queue.front();
				curl_queue.pop();
			}
		}

		if (curl)
			start_new_download(curl);
	}

	int curl_socket_cb(CURL *easy,      /* easy handle */
	                   curl_socket_t s, /* socket */
	                   int action,      /* see values below */
	                   void *userp,     /* private callback pointer */
	                   void *socketp)   /* private socket pointer */
	{
		Socket_Info* info = static_cast<Socket_Info*>(socketp);
		auto ifetcher = ImageFetcher::get();
		
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

	int curl_timer_cb(CURLM *multi, long timeout_ms, void *userp) {
		auto ifetcher = ImageFetcher::get();
		unsigned int timeout = static_cast<unsigned int>(timeout_ms);

		if (ifetcher->timeout_connection.connected())
			ifetcher->timeout_connection.disconnect();

		if (timeout_ms == -1) {
			return 0;
		} else if (timeout_ms == 0) {
			ifetcher->curl_timeout_expired_cb();
		} else {
			auto s = sigc::mem_fun(*ifetcher, &ImageFetcher::curl_timeout_expired_cb);
			ifetcher->timeout_connection = Glib::signal_timeout().connect(s, timeout);
		}

		return 0;
	}

	bool ImageFetcher::curl_timeout_expired_cb() {
		int old_handles = running_handles;
		CURLMcode code = curl_multi_socket_action(curlm, CURL_SOCKET_TIMEOUT, 0, &running_handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_error("Curl socket error from timeout: %s", curl_multi_strerror(code));
		}

		if ( running_handles < old_handles)
			curl_check_info();
		return false;
	}

	bool ImageFetcher::curl_event_cb(Glib::IOCondition condition,
	                                 curl_socket_t s) {
		CURLMcode code;
		int old_handles = running_handles;

		int ev_bitmask = (condition & Glib::IO_IN ? CURL_CSELECT_IN : 0) |
			(condition & Glib::IO_OUT ? CURL_CSELECT_OUT : 0) |
			(condition & Glib::IO_PRI ? CURL_CSELECT_IN : 0) | 
			(condition & Glib::IO_ERR ? CURL_CSELECT_ERR : 0);

		code = curl_multi_socket_action(curlm, s, ev_bitmask, &running_handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_error("Error: Curl socket action error from event: %s",
			        curl_multi_strerror(code));
		}

		if ( running_handles < old_handles)
			curl_check_info();
		if (running_handles == 0) {
			if (timeout_connection.connected())
				timeout_connection.disconnect();
		}

		return true;
	}
	
	void ImageFetcher::curl_setsock(Socket_Info* info, curl_socket_t s,
	                                CURL*, int action) {
		Glib::IOCondition condition = Glib::IO_IN ^ Glib::IO_IN;

		if (action & CURL_POLL_IN)
			condition = Glib::IO_IN;
		if (action & CURL_POLL_OUT)
			condition = condition | Glib::IO_OUT;
		if (action & CURL_POLL_INOUT)
			condition = condition | Glib::IO_OUT | Glib::IO_IN;

		
		if (info->connection.connected())
			info->connection.disconnect();
		const auto slot = sigc::mem_fun(*this, &ImageFetcher::curl_event_cb);
		info->connection = Glib::signal_io().connect( sigc::bind(slot, s),
		                                              info->channel,
		                                              condition );
	}

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

		//#if COLDWIND_WINDOWS
		//info->channel = Glib::IOChannel::create_from_win32_socket(s);
		//#else
		info->channel = Glib::IOChannel::create_from_fd(s);
		//#endif

			if ( G_UNLIKELY( !info->channel )) {
				g_error("Error: Unable to create IOChannel for socket.");
			}

			curl_setsock(info, s, easy, action);
			CURLMcode code = curl_multi_assign(curlm, s, info);
			if (G_UNLIKELY( code != CURLM_OK )) {
				g_error("Error: Unable to assign Socket_Info to socket : %s",
				        curl_multi_strerror(code));
			}
	}

	void ImageFetcher::curl_remsock(Socket_Info* info, curl_socket_t s) {
		if (G_UNLIKELY( !info ))
			return;

		if (G_LIKELY( std::count(active_sockets_.begin(),
		                         active_sockets_.end(), s) > 0 )) {
			if (G_LIKELY( std::count(active_socket_infos_.begin(),
			                         active_socket_infos_.end(), info) > 0 )) {
				// We SHOULD NOT call info->channel->close(). This closes curl's
				// cached connection to the server.
				info->connection.disconnect();
				info->channel.reset();
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

	void ImageFetcher::cleanup_failed_pixmap(std::shared_ptr<Request> request) {
		const bool isThumb = request->is_thumb;
		const bool isGif = request->ext.rfind("gif") != std::string::npos;
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
				std::cerr << "Error closing pixbuf loader for url " << request->url
				          << ": " << e.what() << std::endl;
			} catch (Glib::FileError e) {
				std::cerr << "File error closing pixbuf loader for url " << request->url
				          << ": " << e.what() << std::endl;
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
	}

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

	/* No locks should be held calling into here */
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

	/* Lock already held here */
	void ImageFetcher::curl_check_info() {
		int msgs_left;
		CURL* curl;
		CURLcode code;
		CURLMcode mcode;
		CURLMsg* msg;
		bool download_error = false;
		std::shared_ptr<Request> *request_p;

		do { 
			msg = curl_multi_info_read(curlm, &msgs_left);
			if (msg) {
				switch(msg->msg) {
				case CURLMSG_DONE:
					curl = msg->easy_handle;
					code = msg->data.result;
					if ( G_UNLIKELY(code != CURLE_OK) ) {
						g_warning("Error downloading image: %s\nExtended error message: %s", curl_easy_strerror(code), curl_error_buffer);
						download_error = true;
					}
					curl_easy_getinfo(curl, CURLINFO_PRIVATE, reinterpret_cast<void**>(&request_p));

					mcode = curl_multi_remove_handle(curlm, curl);
					if (G_UNLIKELY(mcode != CURLM_OK)) {
						g_error("While removing curl handle from multi: %s",
						        curl_multi_strerror(mcode));
					}

					if (G_LIKELY(!download_error)) {
						create_pixmap(*request_p);
					} else {
						g_warning("Error downloading %s", (*request_p)->url.c_str());
						cleanup_failed_pixmap(*request_p);
					}

					delete request_p;
					start_new_download(curl);
					break;
				case CURLMSG_NONE:
					g_warning("Warning: Unexpected MSG_NONE from curl info read.");
					break;
				case CURLMSG_LAST:
					g_warning("Warning: Unexpected MSG_LAST from curl info read.");
					break;
				default:
					g_warning("Unexpected message %s from curl info read.", msg->msg);
				}
			}
		} while(msg != NULL && msgs_left > 0);
	}


	ImageFetcher::ImageFetcher() {
		Glib::Mutex::Lock lock(curl_data_mutex);
		signal_pixbuf_ready.connect( sigc::mem_fun(*this, &ImageFetcher::on_pixbuf_ready) );
		
		curl_error_buffer = static_cast<char*>(g_malloc0(CURL_ERROR_SIZE * sizeof(char)));
		curlm = curl_multi_init();

		curl_multi_setopt(curlm, CURLMOPT_SOCKETFUNCTION, &curl_socket_cb);
		curl_multi_setopt(curlm, CURLMOPT_SOCKETDATA, this);
		curl_multi_setopt(curlm, CURLMOPT_TIMERFUNCTION, &curl_timer_cb);
		curl_multi_setopt(curlm, CURLMOPT_TIMERDATA, this);

		for (int i = 0; i < 3; i++) {
			CURL* curl = curl_easy_init();
			curl_queue.push(curl);
		}
	}

	ImageFetcher::~ImageFetcher() {
		Glib::Mutex::Lock lock(curl_data_mutex);
		while (!curl_queue.empty()) {
			curl_easy_cleanup(curl_queue.front());
			curl_queue.pop();
		}
			
		curl_multi_cleanup(curlm);
		g_free(curl_error_buffer);
	}

}
