#ifndef HORIZON_CURL_HPP
#define HORIZON_CURL_HPP
#include <curl/curl.h>
#include <memory>
#include <functional>
#include <deque>
#include <glib.h>

namespace Horizon {

	template <class PrivateData_T>
	class CurlMulti;

	template <class PrivateData_T>
	class CurlEasy {
	public:
		static std::shared_ptr<CurlEasy<PrivateData_T> > create();
		~CurlEasy();
		
		void reset();
		void set_write_function( std::function<std::size_t (const std::string &)> );
		void set_url(const std::string &);
		void set_private(const PrivateData_T&);
		PrivateData_T get_private() const;
		long get_response_code() const;
		void set_fail_on_error();
		void set_error_buffer(char* buffer);
		void set_connect_timeout(long timeout);
		void set_no_signal();
		
		friend class CurlMulti<PrivateData_T>;
	private:
		CurlEasy();
		CurlEasy(const CurlEasy&) = delete;
		CurlEasy& operator=(const CurlEasy&) = delete;

		
		CURL* get();
		CURL* cptr;

		std::function<std::size_t (const std::string &)> *writeback_functor;
		PrivateData_T private_data;
	};

	template <class PrivateData_T>
	class CurlMulti {
	public:
		static std::shared_ptr<CurlMulti<PrivateData_T> > create();
		~CurlMulti();

		std::pair<std::shared_ptr<CurlEasy<PrivateData_T> >, CURLcode> info_read(int *msgs_in_queue);

		void add_handle(std::shared_ptr<CurlEasy<PrivateData_T> > curl);

		void remove_handle(std::shared_ptr<CurlEasy<PrivateData_T> > easy);

		void socket_action(curl_socket_t s, int ev_bitmask, int *handles); 

		void socket_action_timeout(int *handles);

		void multi_assign(curl_socket_t s, void* userdata);

		void set_socket_function( std::function<int (CURL*,
		                                             curl_socket_t,
		                                             int,
		                                             void*,
		                                             void*)> callback,
		                          void* userdata);
		void set_timer_function(std::function<int (CURLM*,
		                                           long,
		                                           void*)> callback,
		                        void* userdata);
		
	private:
		CurlMulti();
		CurlMulti(const CurlMulti&) = delete;
		CurlMulti& operator=(const CurlMulti&) = delete;

		std::function<int (CURL*, curl_socket_t, int, void*, void*)> socket_fn;
		std::function<int (CURLM*, long, void*)> timer_fn;
		std::deque<std::shared_ptr<CurlEasy<PrivateData_T> > > curl_handles;
		CURLM* cptr;
	};


}


#endif
