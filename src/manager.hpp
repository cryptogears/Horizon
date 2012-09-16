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

		std::set<gint64> updatedThreads;
		Glib::Mutex data_mutex;

		Glib::Dispatcher signal_thread_updated;

	private:

		const gint64 MIN_UPDATE_INTERVAL = 10;

		void downloadThread(std::shared_ptr<Thread> thread);

		Curler curler;
		Glib::Mutex curler_mutex;
		std::map<gint64, std::shared_ptr<Thread>> threads;
	};

}
