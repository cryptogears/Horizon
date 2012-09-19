#include <glib.h>
#include <glibmm/thread.h>
#include <glibmm/dispatcher.h>
#include <memory>
#include <map>
#include <set>
#include "thread.hpp"
#include "curler.hpp"


namespace Horizon {

	class Manager {
	public:
		Manager() = default;
		
		bool checkThreads();
		void addThread(std::shared_ptr<Thread> thread);


		bool is_updated_thread() const;
		gint64 pop_updated_thread();

		Glib::Dispatcher signal_thread_updated;

	private:
		mutable Glib::Mutex data_mutex;
		std::set<gint64> updatedThreads;


		void downloadThread(std::shared_ptr<Thread> thread);

		Curler curler;
		Glib::Mutex curler_mutex;
		std::map<gint64, std::shared_ptr<Thread>> threads;
	};

}
