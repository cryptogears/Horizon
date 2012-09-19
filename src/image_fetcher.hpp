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


namespace Horizon {
	struct Socket_Info {
		Glib::RefPtr<Glib::IOChannel> channel;
		sigc::connection connection;
	};

	struct Request {
		std::string hash;
		std::string url;
		bool is_thumb;
	};

	class ImageFetcher {
	public:
		static std::shared_ptr<ImageFetcher> get();
		~ImageFetcher();

		void download_thumb(const std::string &hash, const std::string &url);
		void download_image(const std::string &hash, const std::string &url);

		sigc::signal<void, std::string> signal_thumb_ready;
		sigc::signal<void, std::string> signal_image_ready;

		const Glib::RefPtr<Gdk::Pixbuf> get_thumb(const std::string &hash) const;
		Glib::RefPtr<Gdk::Pixbuf> get_image(const std::string &hash) const;
		bool has_thumb(const std::string &hash) const;
		bool has_image(const std::string &hash) const;

	private:
		ImageFetcher();
		CURLM *curlm;
		std::queue<CURL*> curl_queue;
		std::queue<Request> request_queue;
		sigc::connection timeout_connection;
		std::list<curl_socket_t> active_sockets_;
		std::vector<Socket_Info*> socket_info_cache_;
		std::list<Socket_Info*> active_socket_infos_;
		int running_handles;

		bool create_pixmap(bool isThumb, const std::string &hash);
		void start_new_download(CURL *curl);
		/* Hash to Image Pixbuf. 
		   Expose */
		mutable Glib::Mutex images_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > images;
		std::map<std::string, Glib::RefPtr<Gio::MemoryInputStream> > image_streams;

		/* Hash to Thumbnail Pixbuf 
		   Expose */
		mutable Glib::Mutex thumbs_mutex;
		std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > thumbs;
		std::map<std::string, Glib::RefPtr<Gio::MemoryInputStream> > thumb_streams;
		
		std::map<CURL*, bool> curl_is_thumb;

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
