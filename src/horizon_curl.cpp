#include "horizon_curl.hpp"
#include <algorithm>
#include <iostream>
#include "image_fetcher.hpp"

namespace {
	extern "C" {
		static std::size_t horizon_curl_write_callback(char* buf,
		                                               size_t size,
		                                               size_t nmemb,
		                                               void* userdata) {
			auto functor_ptr = static_cast<std::function<std::size_t (const std::string &)> *>(userdata);

			const std::string str_buf(buf, size*nmemb);
			
			return functor_ptr->operator()(str_buf);
		}
	}
}

namespace Horizon {

	template <class PrivateData_T>
	std::shared_ptr<CurlEasy<PrivateData_T> > CurlEasy<PrivateData_T>::create() {
		return std::shared_ptr<CurlEasy<PrivateData_T> >(new CurlEasy<PrivateData_T>());
	}

	template <class PrivateData_T>
	CurlEasy<PrivateData_T>::CurlEasy() :
		cptr(curl_easy_init()),
		writeback_functor(nullptr)
	{
	}

	template <class PrivateData_T>
	CurlEasy<PrivateData_T>::~CurlEasy() {
		curl_easy_cleanup(cptr);
		if (writeback_functor)
			delete writeback_functor;
	}

	template <class PrivateData_T>
	CURL* CurlEasy<PrivateData_T>::get() {
		return cptr;
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::reset() {
		curl_easy_reset(cptr);

		if (writeback_functor) {
			delete writeback_functor;
			writeback_functor = nullptr;
		}
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_url( const std::string &url ) {
		curl_easy_setopt(cptr, CURLOPT_URL, url.c_str());
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_write_function( std::function<std::size_t (const std::string &)> functor ) {
		if (writeback_functor)
			delete writeback_functor;
		
		writeback_functor = new std::function<std::size_t (const std::string &)>(functor);

		curl_easy_setopt(cptr, CURLOPT_WRITEFUNCTION, &horizon_curl_write_callback);
		curl_easy_setopt(cptr, CURLOPT_WRITEDATA, writeback_functor);
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_private(const PrivateData_T &userdata) {
		private_data = userdata;
		curl_easy_setopt(cptr, CURLOPT_PRIVATE, &private_data);
	}

	template <class PrivateData_T>
	PrivateData_T CurlEasy<PrivateData_T>::get_private() const {
		return private_data;
	}

	template <class PrivateData_T>
	long CurlEasy<PrivateData_T>::get_response_code() const {
		long code = 0;
		curl_easy_getinfo(cptr, CURLINFO_RESPONSE_CODE, &code);

		return code;
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_fail_on_error() {
		curl_easy_setopt(cptr, CURLOPT_FAILONERROR, 1);
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_error_buffer(char *buffer) {
		curl_easy_setopt(cptr, CURLOPT_ERRORBUFFER, buffer);
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_connect_timeout(long timeout) {
		curl_easy_setopt(cptr, CURLOPT_CONNECTTIMEOUT, timeout);
	}

	template <class PrivateData_T>
	void CurlEasy<PrivateData_T>::set_no_signal() {
		curl_easy_setopt(cptr, CURLOPT_NOSIGNAL, 1);
	}

	template <class PrivateData_T>
	std::shared_ptr<CurlMulti<PrivateData_T> > CurlMulti<PrivateData_T>::create() {
		return std::shared_ptr<CurlMulti<PrivateData_T> >(new CurlMulti<PrivateData_T>());
	}

	template <class PrivateData_T>
	CurlMulti<PrivateData_T>::CurlMulti() :
		cptr(curl_multi_init())
	{
	}

	template <class PrivateData_T>
	CurlMulti<PrivateData_T>::~CurlMulti() {
		CURLM *multi = cptr;
		std::for_each(curl_handles.begin(),
		              curl_handles.end(),
		              [&multi](std::shared_ptr<CurlEasy<PrivateData_T> > easy) {
			              curl_multi_remove_handle(multi, easy->get());
		              });
		curl_handles.clear();

		curl_multi_cleanup(cptr);
	}

	template <class PrivateData_T>
	std::pair<std::shared_ptr<CurlEasy<PrivateData_T> >, CURLcode> CurlMulti<PrivateData_T>::info_read(int *msgs_in_queue) {
		std::shared_ptr<CurlEasy<PrivateData_T> > easy;
		CURLcode res = CURL_LAST;
		CURL* curl = nullptr;
		auto iter = curl_handles.end();
		CURLMsg* msg = curl_multi_info_read(cptr, msgs_in_queue);
		if (msg) {
			switch(msg->msg) {
			case CURLMSG_DONE:
				curl = msg->easy_handle;
				res = msg->data.result;

				iter = std::find_if(curl_handles.begin(),
				                    curl_handles.end(),
				                    [&curl](std::shared_ptr<CurlEasy<PrivateData_T> > ptr) {
					                    return (ptr->get() == curl);
				                    });
				if (iter == curl_handles.end()) {
					g_error("multi_info_read returned unknown curl handle");
				} else {
					easy = *iter;
				}
				break;
			case CURLMSG_NONE:
			case CURLMSG_LAST:
			default:
				g_warning("Unexpected CURLMsg %d", msg->msg);
				break;
			}
		}

		return std::make_pair(easy, res);
	}

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::socket_action(curl_socket_t s, int ev_bitmask, int *handles) {
		CURLMcode code = curl_multi_socket_action(cptr, s, ev_bitmask, handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl socket action error: %s", curl_multi_strerror(code));
		}
	}

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::socket_action_timeout(int *handles) {
		CURLMcode code = curl_multi_socket_action(cptr, CURL_SOCKET_TIMEOUT, 0, handles);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl socket error from timeout: %s",
			          curl_multi_strerror(code));
		}
	}

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::multi_assign(curl_socket_t s, void* userdata) {
		CURLMcode code = curl_multi_assign(cptr, s, userdata);
		if (G_UNLIKELY( code != CURLM_OK )) {
			g_warning("Curl multi error from assign: %s",
			          curl_multi_strerror(code));
		}
	}

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::add_handle(std::shared_ptr<CurlEasy<PrivateData_T> > curl) {
		curl_handles.push_back(curl);

		curl_multi_add_handle(cptr, curl->get());
	}

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::remove_handle(std::shared_ptr<CurlEasy<PrivateData_T> > easy) {

		auto iter = std::find(curl_handles.begin(),
		                      curl_handles.end(),
		                      easy);

		if (iter != curl_handles.end()) {
			CURLMcode code = curl_multi_remove_handle(cptr, easy->get());
			if (G_UNLIKELY( code != CURLM_OK )) {
				g_warning("Curl multi error from remove handle: %s",
				          curl_multi_strerror(code));
			}

			curl_handles.erase(iter);
		} else {
			std::cerr << "Error: CurlMulti removed handle "
			          << "that we have no record of." << std::endl;
		}
	}


	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::set_socket_function( std::function<int (CURL*,
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

	template <class PrivateData_T>
	void CurlMulti<PrivateData_T>::set_timer_function(std::function<int (CURLM*,
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

