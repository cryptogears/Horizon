#ifndef IMAGE_FETCHER_HPP
#define IMAGE_FETCHER_HPP
#include <memory>
#include <curl/curl.h>
#include <glib.h>
#include <queue>
#include <map>
#include <functional>
#include <glibmm/thread.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <sigc++/sigc++.h>
#include <giomm/memoryinputstream.h>
#include <glibmm/timer.h>
#include <glibmm/iochannel.h>
#include <glibmm/miscutils.h>
#include <glibmm/main.h>
#include <gdkmm/pixbufloader.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threads.h>
#include "image_cache.hpp"
#include "horizon_curl.hpp"

#ifdef HAVE_EV___H
#include <ev++.h>
#else
#include <libev/ev++.h>
#endif

namespace Horizon {
	struct Socket_Info {
		std::shared_ptr<ev::io> w;
	};

	enum FETCH_TYPE {FOURCHAN, CATALOG};

	struct Request {
		Glib::RefPtr< Gdk::PixbufLoader > loader;
		Glib::RefPtr< Gio::MemoryInputStream > istream;
		Glib::RefPtr<Post> post;
		std::string hash;
		std::string url;
		std::string ext;
		bool is_thumb;

		std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared_functor;
		std::function<void (int, int, int, int)> area_updated_functor;
		sigc::connection area_prepared_connection;
		sigc::connection area_updated_connection;
	};

	class ImageFetcher {
	public:

		static std::shared_ptr<ImageFetcher> get(FETCH_TYPE);
		~ImageFetcher();

		void download_thumb(const Glib::RefPtr<Post> &);
		void download_image(const Glib::RefPtr<Post> &,
		                    std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared = nullptr,
		                    std::function<void (int, int, int, int)> area_updated = nullptr);

		sigc::signal<void, std::string> signal_thumb_ready;
		sigc::signal<void, std::string> signal_image_ready;

		const Glib::RefPtr<Gdk::Pixbuf> get_thumb(const std::string &hash) const;
		const Glib::RefPtr<Gdk::Pixbuf> get_image(const std::string &hash) const;
		const Glib::RefPtr<Gdk::PixbufAnimation> get_animation(const std::string &hash) const;
		bool has_thumb(const std::string &hash) const;
		bool has_image(const std::string &hash) const;
		bool has_animation(const std::string &hash) const;

	private:
		ImageFetcher();
		std::shared_ptr<ImageCache> image_cache;

		void on_cache_result(const Glib::RefPtr<Gdk::PixbufLoader>&,
		                     std::shared_ptr<Request>);

		// Mutex wraps curl_queue, request_queue
		mutable Glib::Mutex curl_data_mutex;
		char* curl_error_buffer;
		std::shared_ptr< CurlMulti< std::shared_ptr<Request> > > curl_multi;

		std::queue<std::shared_ptr<CurlEasy<std::shared_ptr<Request> > > > curl_queue;
		std::queue<std::shared_ptr<Request> >  request_queue;

		sigc::connection timeout_connection;
		std::list<curl_socket_t> active_sockets_;
		std::vector<Socket_Info*> socket_info_cache_;
		std::list<Socket_Info*> active_socket_infos_;
		int running_handles;

		mutable Glib::Mutex pixbuf_mutex;
		Glib::Dispatcher signal_pixbuf_ready;
		std::map<std::shared_ptr<Request>, Glib::RefPtr<Gdk::Pixbuf> > pixbuf_map;
		std::map<std::shared_ptr<Request>, Glib::RefPtr<Gdk::PixbufAnimation> > pixbuf_animation_map;

		Glib::Dispatcher signal_pixbuf_updated;
		void on_pixbuf_updated();
		void on_area_prepared(std::shared_ptr<Request> request);
		void on_area_updated(int x, int y, int width, int height, std::shared_ptr<Request> request);
		std::deque<std::function<void ()> > pixbuf_update_queue;


		void on_pixbuf_ready();
		void remove_pending(std::shared_ptr<Request> request);
		void cleanup_failed_pixmap(std::shared_ptr<Request> request, bool is_404);
		void create_pixmap(std::shared_ptr<Request> request);
		void start_new_download();

		/* Hash to Image Pixbuf. 
		   Expose */
		mutable Glib::Mutex images_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > images;
		std::map<std::string, Glib::RefPtr<Gdk::PixbufAnimation> > animations;
		std::set<std::string> pending_images;

		/* Hash to Thumbnail Pixbuf 
		   Expose */
		mutable Glib::Mutex thumbs_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > thumbs;
		std::set<std::string> pending_thumbs;
		
		/* Curl socket eventing methods */
		void curl_addsock(curl_socket_t s, CURL *easy, int action);
		void curl_setsock(Socket_Info* info, curl_socket_t s, 
		                  CURL* curl, int action);
		void curl_remsock(Socket_Info* info, curl_socket_t s);
		void curl_check_info();
		bool curl_timeout_expired_cb();
		void curl_event_cb(ev::io &w, int);

		/* Libev */
		Glib::Threads::Thread *ev_thread;
		mutable Glib::Threads::Mutex ev_mutex;
		ev::dynamic_loop ev_loop;
		void             looper();

		ev::async        kill_loop_w;
		void             on_kill_loop_w(ev::async &w, int);

		ev::async        queue_w;
		void             on_queue_w(ev::async &w, int);

		ev::timer        timeout_w;
		void             on_timeout_w(ev::timer &w, int);


		std::size_t curl_writeback(const std::string &str_buf,
		                           std::shared_ptr<Request> request);

		friend int curl_socket_cb(CURL *easy,      /* easy handle */   
		                          curl_socket_t s, /* socket */   
		                          int action,      /* see values below */   
		                          void *userp,     /* private callback pointer */
		                          void *socketp);  /* private socket pointer */
		
		friend int curl_timer_cb(CURLM *multi,    /* multi handle */
		                         long timeout_ms, /* see above */
		                         void *userp);    /* private callback
		                                             pointer */

	};
	
	int curl_socket_cb(CURL *easy,      /* easy handle */   
	                   curl_socket_t s, /* socket */   
	                   int action,      /* see values below */   
	                   void *userp,     /* private callback pointer */   
	                   void *socketp);  /* private socket pointer */
	
	int curl_timer_cb(CURLM *multi,    /* multi handle */
	                  long timeout_ms, /* see above */
	                  void *userp);    /* private callback
	                                      pointer */
	
}



#endif
