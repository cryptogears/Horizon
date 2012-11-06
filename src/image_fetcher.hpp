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

		/* Old interface */
		void download_thumb(const Glib::RefPtr<Post> &) G_GNUC_DEPRECATED;
		void download_image(const Glib::RefPtr<Post> &,
		                    std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared = nullptr,
		                    std::function<void (int, int, int, int)> area_updated = nullptr) G_GNUC_DEPRECATED;

		sigc::signal<void, std::string> signal_thumb_ready;
		sigc::signal<void, std::string> signal_image_ready;

		const Glib::RefPtr<Gdk::Pixbuf> get_thumb(const std::string &hash) const G_GNUC_DEPRECATED;
		const Glib::RefPtr<Gdk::Pixbuf> get_image(const std::string &hash) const G_GNUC_DEPRECATED;
		const Glib::RefPtr<Gdk::PixbufAnimation> get_animation(const std::string &hash) const G_GNUC_DEPRECATED;

		bool has_thumb(const std::string &hash) const G_GNUC_DEPRECATED;
		bool has_image(const std::string &hash) const G_GNUC_DEPRECATED;
		bool has_animation(const std::string &hash) const G_GNUC_DEPRECATED;

		/* New interface */
		void download(const Glib::RefPtr<Post> &,
		              std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &loader)> callback,
		              bool get_thumb = true,
		              std::function<void (Glib::RefPtr<Gdk::Pixbuf>)> area_prepared_cb = nullptr,
		              std::function<void (int, int, int, int)> area_updated_cb = nullptr
 		              );

	private:
		ImageFetcher();

		/* Image Cache handles on-disk images */
		std::shared_ptr<ImageCache> image_cache;
		void on_cache_result(const Glib::RefPtr<Gdk::PixbufLoader>&,
		                     std::shared_ptr<Request>);

		void start_new_download();
		void cleanup_failed_pixmap(std::shared_ptr<Request> request, bool is_404);
		void create_pixmap(std::shared_ptr<Request> request);
		void remove_pending(std::shared_ptr<Request> request);
		
		/* New Interface */
		std::multimap<std::string,
		              std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &)>
		              >                                    request_cb_map;
		mutable Glib::Threads::RWLock                      request_cb_rwlock;
		std::deque<std::shared_ptr<Request> >              new_request_queue;
		mutable Glib::Threads::RWLock                      request_queue_rwlock;
		std::deque<std::function<void ()> >                cb_queue;
		mutable Glib::Threads::RWLock                      cb_queue_rwlock;
		sigc::connection                                   cb_queue_idle;

		bool process_cb_queue();
		bool add_request_cb(const std::string &request_key,
		                    std::function<void (const Glib::RefPtr<Gdk::PixbufLoader> &)> cb);
		bool bind_loader_to_callbacks(const std::string &request_key,
		                              const Glib::RefPtr<Gdk::PixbufLoader> &loader);
		void add_request(const std::shared_ptr<Request> &);



		mutable Glib::Mutex pixbuf_mutex;
		Glib::Dispatcher signal_pixbuf_ready;
		void on_pixbuf_ready();
		std::map<std::shared_ptr<Request>, Glib::RefPtr<Gdk::Pixbuf> > pixbuf_map;
		std::map<std::shared_ptr<Request>, Glib::RefPtr<Gdk::PixbufAnimation> > pixbuf_animation_map;

		/* Progressive Loading callbacks */
		void signal_pixbuf_updated();
		Glib::Threads::Mutex pixbuf_updated_idle_mutex;
		sigc::connection pixbuf_updated_idle;
		bool on_pixbuf_updated();
		void on_area_prepared(std::shared_ptr<Request> request);
		void on_area_updated(int x, int y, int width, int height, std::shared_ptr<Request> request);
		std::deque<std::function<void ()> > pixbuf_update_queue;

		/* Old interface state */
		mutable Glib::Mutex images_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > images;
		std::map<std::string, Glib::RefPtr<Gdk::PixbufAnimation> > animations;
		std::set<std::string> pending_images;

		mutable Glib::Mutex thumbs_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > thumbs;
		std::set<std::string> pending_thumbs;

		/* old interface: Requests */
		// Uses curl_data_mutex
		std::queue<std::shared_ptr<Request> >  request_queue;
		

		/* Curl socket eventing methods */
		void curl_addsock(curl_socket_t s, CURL *easy, int action);
		void curl_setsock(Socket_Info* info, curl_socket_t s, 
		                  CURL* curl, int action);
		void curl_remsock(Socket_Info* info, curl_socket_t s);
		void curl_check_info();
		bool curl_timeout_expired_cb();
		void curl_event_cb(ev::io &w, int);

		/* Curl state */
		mutable Glib::Mutex curl_data_mutex;
		char* curl_error_buffer;
		std::shared_ptr< CurlMulti< std::shared_ptr<Request> > > curl_multi;
		std::queue<std::shared_ptr<CurlEasy<std::shared_ptr<Request> > > > curl_queue;

		sigc::connection timeout_connection;
		std::list<curl_socket_t> active_sockets_;
		std::vector<Socket_Info*> socket_info_cache_;
		std::list<Socket_Info*> active_socket_infos_;
		int running_handles;

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
