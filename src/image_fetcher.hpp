#ifndef IMAGE_FETCHER_HPP
#define IMAGE_FETCHER_HPP
#include <memory>
#include <curl/curl.h>
#include <glib.h>
#include <queue>
#include <map>
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


namespace Horizon {
	struct Socket_Info {
		Glib::RefPtr<Glib::IOChannel> channel;
		sigc::connection connection;
	};

	struct Request {
		std::string hash;
		std::string url;
		std::string ext;
		bool is_thumb;
	};

	class ImageFetcher {
	public:
		static std::shared_ptr<ImageFetcher> get();
		~ImageFetcher();

		void download_thumb(const std::string &hash, const std::string &url);
		void download_image(const std::string &hash, const std::string &url, const std::string &ext);

		sigc::signal<void, std::string> signal_thumb_ready;
		sigc::signal<void, std::string> signal_image_ready;

		const Glib::RefPtr<Gdk::Pixbuf> get_thumb(const std::string &hash) const;
		const Glib::RefPtr<Gdk::Pixbuf> get_image(const std::string &hash) const;
		const Glib::RefPtr<Gdk::PixbufAnimation> get_animation(const std::string &hash) const;
		bool has_thumb(const std::string &hash) const;
		bool has_image(const std::string &hash) const;

	private:
		ImageFetcher();
		CURLM *curlm;

		mutable Glib::Mutex curl_data_mutex;
		char* curl_error_buffer;
		std::queue<CURL*> curl_queue;
		std::queue<Request*> request_queue;
		sigc::connection timeout_connection;
		std::list<curl_socket_t> active_sockets_;
		std::vector<Socket_Info*> socket_info_cache_;
		std::list<Socket_Info*> active_socket_infos_;
		int running_handles;

		mutable Glib::Mutex pixbuf_mutex;
		Glib::Dispatcher signal_pixbuf_ready;
		std::map<const Request*, Glib::RefPtr<Gdk::Pixbuf> > pixbuf_map;
		std::map<const Request*, Glib::RefPtr<Gdk::PixbufAnimation> > pixbuf_animation_map;
		void on_pixbuf_ready();

		bool create_pixmap(const Request* request);
		void start_new_download(CURL *curl);
		/* Hash to Image Pixbuf. 
		   Expose */
		mutable Glib::Mutex images_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > images;
		std::map<std::string, Glib::RefPtr<Gdk::PixbufAnimation> > animations;
		std::map<std::string, Glib::RefPtr<Gdk::PixbufLoader> > image_streams;

		/* Hash to Thumbnail Pixbuf 
		   Expose */
		mutable Glib::Mutex thumbs_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > thumbs;
		std::map<std::string, Glib::RefPtr<Gdk::PixbufLoader> > thumb_streams;
		
		friend std::size_t horizon_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata);

		friend int curl_socket_cb(CURL *easy,      /* easy handle */   
		                          curl_socket_t s, /* socket */   
		                          int action,      /* see values below */   
		                          void *userp,     /* private callback pointer */
		                          void *socketp);  /* private socket pointer */
		
		friend int curl_timer_cb(CURLM *multi,    /* multi handle */
		                         long timeout_ms, /* see above */
		                         void *userp);    /* private callback
		                                             pointer */

		friend void pixbuf_ready_callback(GObject *source_object,
		                                  GAsyncResult *res,
		                                  gpointer user_data);

		void curl_addsock(curl_socket_t s, CURL *easy, int action);
		void curl_setsock(Socket_Info* info, curl_socket_t s, 
		                  CURL* curl, int action);
		void curl_remsock(Socket_Info* info, curl_socket_t s);
		void curl_check_info();
		bool curl_timeout_expired_cb();
		bool curl_event_cb(Glib::IOCondition condition, curl_socket_t s);

	};
	
	std::size_t horizon_curl_writeback(char *ptr, size_t size, size_t nmemb, void *userdata);

	
	int curl_socket_cb(CURL *easy,      /* easy handle */   
	                   curl_socket_t s, /* socket */   
	                   int action,      /* see values below */   
	                   void *userp,     /* private callback pointer */   
	                   void *socketp);  /* private socket pointer */
	
	int curl_timer_cb(CURLM *multi,    /* multi handle */
	                  long timeout_ms, /* see above */
	                  void *userp);    /* private callback
	                                      pointer */
	
	void pixbuf_ready_callback(GObject *, GAsyncResult *res, gpointer user_data);
}



#endif
