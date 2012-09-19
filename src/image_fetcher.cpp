#include "image_fetcher.hpp"

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
			g_critical("ImageFetcher::get_thumb called for an image not yet downloaded");
			return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
		}
	}

	bool ImageFetcher::has_thumb(const std::string &hash) const {
		Glib::Mutex::Lock lock(thumbs_mutex);
		
		auto iter = thumbs.find(hash);
		if ( iter != thumbs.end() ) {
			return true;
		} else {
			return false;
		}
	}

	static void horizon_destroy_notify(gpointer ptr) {
		g_free(ptr);
	}

	std::size_t horizon_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata) {
		gpointer data = g_memdup(static_cast<gpointer>(ptr), size*nmemb);
		std::string hash = static_cast<char*>(userdata);
		std::shared_ptr<ImageFetcher> ifetcher = ImageFetcher::get();
		auto iter = ifetcher->thumb_streams.find(hash);
		if ( iter != ifetcher->thumb_streams.end() ) {
			Glib::RefPtr< Gio::MemoryInputStream >  stream = iter->second;
			stream->add_data(data, size*nmemb, horizon_destroy_notify);
		} else {
			g_critical("horizon_curl_writeback called with unknown hash %s.", hash.c_str());
		}

		return size*nmemb;
	}

	
	
	void ImageFetcher::start_new_download(CURL *curl) {
		if (request_queue.empty()) {
			curl_queue.push(curl);
		} else {
			auto request = request_queue.front(); request_queue.pop();
			const std::string hash = request.hash;
			const std::string url = request.url;
			const bool is_thumb = request.is_thumb;
			curl_easy_reset(curl);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, horizon_curl_writeback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, hash.c_str());
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_PRIVATE, hash.c_str());
			if (is_thumb) {
				curl_is_thumb.insert({curl, true});
				thumb_streams.insert({hash, Gio::MemoryInputStream::create()});
			}
			curl_multi_add_handle(curlm, curl);
		}
	}

	void ImageFetcher::download_thumb(const std::string &hash, const std::string &url) {
		Request req;
		req.hash = hash;
		req.url = url;
		req.is_thumb = true;
		request_queue.push(req);
		if ( ! curl_queue.empty() ) {
			CURL* curl = curl_queue.front();
			curl_queue.pop();
			start_new_download(curl);
		} 
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
		CURLMcode code = curl_multi_socket_action(curlm, CURL_SOCKET_TIMEOUT, 0, &running_handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_error("Curl socket error from timeout: %s", curl_multi_strerror(code));
		}

		curl_check_info();
		return false;
	}

	bool ImageFetcher::curl_event_cb(Glib::IOCondition condition,
	                                 curl_socket_t s) {
		CURLMcode code;

		int action = (condition & Glib::IO_IN ? CURL_CSELECT_IN : 0) |
			(condition & Glib::IO_OUT ? CURL_CSELECT_OUT : 0);
		code = curl_multi_socket_action(curlm, s, action, &running_handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_error("Error: Curl socket action error from event: %s",
			        curl_multi_strerror(code));
		}
		
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

	void pixbuf_ready_callback (GObject *source_object, 
	                            GAsyncResult *res,
	                            gpointer user_data) {
		auto p = static_cast<std::pair<ImageFetcher*, std::string>* >(user_data);
		
		GError *error = NULL;
		GdkPixbuf *cpixbuf = gdk_pixbuf_new_from_stream_finish(res, &error);
		if (error == NULL) {
			Glib::RefPtr<Gdk::Pixbuf> pixbuf = Glib::wrap(cpixbuf);
			p->first->thumbs.insert({p->second, pixbuf});
			p->first->signal_thumb_ready(p->second);
		} else {
			g_warning("Error creating pixbuf: %s", error->message);
		}
		delete p;
	}

	bool ImageFetcher::create_pixmap(bool isThumb, const std::string &hash) {
		if ( isThumb ) {
			// This is a thumbnail
			auto iter = thumb_streams.find(hash);
			if (iter != thumb_streams.end()) {
				try {
					//auto pixbuf = Gdk::Pixbuf::create_from_stream(iter->second);
					std::pair<ImageFetcher*, std::string>* mypair = new std::pair<ImageFetcher*, std::string>();
					mypair->first = this;
					mypair->second = hash;
					gdk_pixbuf_new_from_stream_async(G_INPUT_STREAM(iter->second->gobj()), NULL, pixbuf_ready_callback, mypair);
				} catch (Glib::Error e) {
					g_error("Error loading pixmap: %s", e.what().c_str());
				} 
				thumb_streams.erase(hash);

				return true;
			} else {
				g_error("Hash not in thumbstreams");
			}
		} else {
			// This is a fullsize image
			auto iter = image_streams.find(hash);
			if ( iter != image_streams.end()) {
				images.insert({hash, Gdk::Pixbuf::create_from_stream(iter->second)});
				signal_image_ready(hash);
				image_streams.erase(iter);
				return true;
			} else {
				g_error("Hash not in imagestreams");
			}
		}
		return false;
	}

	void ImageFetcher::curl_check_info() {
		int msgs_left;
		CURL* curl;
		CURLcode code;
		CURLMcode mcode;
		CURLMsg* msg;
		bool download_error = false;
		char* hash;

		do { 
			msg = curl_multi_info_read(curlm, &msgs_left);
			if (msg) {
				switch(msg->msg) {
				case CURLMSG_DONE:
					curl = msg->easy_handle;
					code = msg->data.result;
					if ( code != CURLE_OK ) {
						g_warning("Error downloading image: %s", curl_easy_strerror(code));
						download_error = true;
					}

					mcode = curl_multi_remove_handle(curlm, curl);
					if (G_UNLIKELY(mcode != CURLM_OK)) {
						g_error("While removing curl handle from multi: %s",
						        curl_multi_strerror(mcode));
					}

					curl_easy_getinfo(curl, CURLINFO_PRIVATE, &hash);
					if (G_LIKELY(!download_error)) {
						auto iter_is_thumb = curl_is_thumb.find(curl);
						if (iter_is_thumb != curl_is_thumb.end()) {
							bool isThumb = iter_is_thumb->second;
							create_pixmap(isThumb, hash);
							curl_is_thumb.erase(iter_is_thumb);
						} else {
							g_error("Hash not in iter_is_thumb");
						}
					} else {
						g_warning("Error downloading an image.");
					}

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
		while (!curl_queue.empty()) {
			curl_easy_cleanup(curl_queue.front());
			curl_queue.pop();
		}
			
		curl_multi_cleanup(curlm);
	}

}
