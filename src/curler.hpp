#include <curl/curl.h>
#include <json-glib/json-glib.h>
#include <glibmm/dispatcher.h>
#include "thread.hpp"
#include <glibmm/datetime.h>
#include <stdexcept>

#include "thread_summary.hpp"

namespace Horizon {

	class Thread404 : public std::exception {
		public:
		Thread404() = default;
	};

	class Concurrency : public std::exception {
		public:
		Concurrency() = default;
	};

	class TooFast : public std::exception {
		public:
		TooFast() = default;
	};
   
	class CurlException : public std::runtime_error {
	public:
		CurlException(std::string estr) : std::runtime_error(estr) {};
	};


	class Curler {
	public:
		Curler();
		~Curler();

		/**
		   Downloads and parses the thread.
		*/
		std::list<Glib::RefPtr<Post> > pullThread(std::shared_ptr<Thread> thread);
		std::list<Glib::RefPtr<ThreadSummary> > pullBoard(const std::string& url,
		                                                  const std::string &board);

		void thread_writeback(const void* data, gssize len);
		Glib::Dispatcher thread_downloaded;

	private:
		CURL *curl;
		bool amWorking;
		JsonParser *parser;
		JsonReader *reader;
		std::string threadBuffer;
		Glib::DateTime last_pull;
	};
}

