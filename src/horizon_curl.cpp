#include "horizon_curl.hpp"
#include <algorithm>
#include <iostream>

namespace Horizon {

	std::shared_ptr<CurlEasy> CurlEasy::create() {
		return std::shared_ptr<CurlEasy>(new CurlEasy());
	}

	CurlEasy::CurlEasy() :
		cptr(curl_easy_init())
	{
	}

	CurlEasy::~CurlEasy() {
		curl_easy_cleanup(cptr);
	}

	CURL* CurlEasy::get() {
		return cptr;
	}

	void CurlEasy::reset() {
		curl_easy_reset(cptr);
	}

	void CurlEasy::set_url( const std::string &url ) {
		curl_easy_setopt(cptr, CURLOPT_URL, url.c_str());
	}

	void CurlEasy::set_write_function( std::size_t (*callback)(char*, size_t, size_t, void*),
	                                   void* userdata ) {
		curl_easy_setopt(cptr, CURLOPT_WRITEFUNCTION, callback);
		curl_easy_setopt(cptr, CURLOPT_WRITEDATA, userdata);
	}

	void CurlEasy::set_private(void *userdata) {
		curl_easy_setopt(cptr, CURLOPT_PRIVATE, userdata);
	}

	void CurlEasy::set_fail_on_error() {
		curl_easy_setopt(cptr, CURLOPT_FAILONERROR, 1);
	}

	void CurlEasy::set_error_buffer(char *buffer) {
		curl_easy_setopt(cptr, CURLOPT_ERRORBUFFER, buffer);
	}

	void CurlEasy::set_connect_timeout(long timeout) {
		curl_easy_setopt(cptr, CURLOPT_CONNECTTIMEOUT, timeout);
	}

	void CurlEasy::set_no_signal() {
		curl_easy_setopt(cptr, CURLOPT_NOSIGNAL, 1);
	}

	std::shared_ptr<CurlMulti> CurlMulti::create() {
		return std::shared_ptr<CurlMulti>(new CurlMulti());
	}

	CurlMulti::CurlMulti() :
		cptr(curl_multi_init())
	{
	}

	CurlMulti::~CurlMulti() {
		CURLM *multi = cptr;
		std::for_each(curl_handles.begin(),
		              curl_handles.end(),
		              [&multi](std::shared_ptr<CurlEasy> easy) {
			              curl_multi_remove_handle(multi, easy->get());
		              });
		curl_handles.clear();

		curl_multi_cleanup(cptr);
	}

	CURLMsg* CurlMulti::info_read(int *msgs_in_queue) {
		return curl_multi_info_read(cptr, msgs_in_queue);
	}

	void CurlMulti::socket_action(curl_socket_t s, int ev_bitmask, int *handles) {
		CURLMcode code = curl_multi_socket_action(cptr, s, ev_bitmask, handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl socket action error: %s", curl_multi_strerror(code));
		}
	}

	void CurlMulti::socket_action_timeout(int *handles) {
		CURLMcode code = curl_multi_socket_action(cptr, CURL_SOCKET_TIMEOUT, 0, handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl socket error from timeout: %s",
			          curl_multi_strerror(code));
		}
	}

	void CurlMulti::multi_assign(curl_socket_t s, void* userdata) {
		CURLMcode code = curl_multi_assign(cptr, s, userdata);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl multi error from assign: %s",
			          curl_multi_strerror(code));
		}
	}

	void CurlMulti::add_handle(std::shared_ptr<CurlEasy> curl) {
		curl_handles.push_back(curl);

		curl_multi_add_handle(cptr, curl->get());
	}

	std::shared_ptr<CurlEasy> CurlMulti::remove_handle(CURL* curl) {
		CURLMcode code = curl_multi_remove_handle(cptr, curl);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl multi error from remove handle: %s",
			          curl_multi_strerror(code));
		}

		auto iter = std::find_if(curl_handles.begin(),
		                         curl_handles.end(),
		                         [&curl](std::shared_ptr<CurlEasy> easy) {
			                         return ( ( easy->get() ) == curl );
		                         });

		if (iter != curl_handles.end()) {
			std::shared_ptr<CurlEasy> easy = *iter;
			curl_handles.erase(iter);
			return easy;
		} else {
			std::cerr << "Error: CurlMulti removed handle "
			          << "that we have no record of." << std::endl;
			return nullptr;
		}
	}


	void CurlMulti::set_socket_function( std::function<int (CURL*,
	                                                        curl_socket_t,
	                                                        int,
	                                                        void*,
	                                                        void*)> callback,
	                                     void* userdata) {
		socket_fn = callback;
		int (**cbptr)(CURL*, curl_socket_t, int, void*, void*) = nullptr;

		cbptr = callback.target<int (*)(CURL*, curl_socket_t, int, void*, void*)>();
		if (cbptr != nullptr)
			curl_multi_setopt(cptr, CURLMOPT_SOCKETFUNCTION, *cbptr );
		else
			g_error("Unable to set the curl socket callback function.");
		
		curl_multi_setopt(cptr, CURLMOPT_SOCKETDATA, userdata);
	}

	void CurlMulti::set_timer_function(std::function<int (CURLM*,
	                                                      long,
	                                                      void*)> callback,
	                                   void* userdata) {
		timer_fn = callback;
		int (**cbptr)(CURLM*, long, void*) = nullptr;
		cbptr = callback.target<int (*)(CURLM*, long, void*)>();
		if (cbptr != nullptr)
			curl_multi_setopt(cptr, CURLMOPT_TIMERFUNCTION, *cbptr);
		else
			g_error("Unable to set the curl timer function. The functor must be"
			        " a plain C function pointer.");

		curl_multi_setopt(cptr, CURLMOPT_TIMERDATA, userdata);
	}
}
