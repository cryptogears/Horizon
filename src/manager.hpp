#include <glib.h>
#include <glibmm/threads.h>
#include <glibmm/dispatcher.h>
#include <memory>
#include <map>
#include <set>
#include "thread.hpp"
#include "curler.hpp"
#include "thread_summary.hpp"

namespace Horizon {

	class Manager {
	public:
		Manager() = default;
		~Manager();

		/* Interface for catalogs */
		bool update_catalogs();
		bool add_catalog_board(const std::string &board);
		bool remove_catalog_board(const std::string &board);
		bool for_each_catalog_board(std::function<bool (const std::string&)>) const;
		const std::list<Glib::RefPtr<ThreadSummary> >& get_catalog(const std::string& board) const;
		bool is_updated_catalog() const;
		const std::string pop_updated_catalog_board();
		Glib::Dispatcher signal_catalog_updated;


		/* Interface for threads */
		bool update_threads();
		void add_thread(std::shared_ptr<Thread> thread);
		void remove_thread(const gint64 id);

		bool is_updated_thread() const;
		gint64 pop_updated_thread();
		Glib::Dispatcher signal_thread_updated;

	private:
		/* Threaded Loops */
		void threads_loop() G_GNUC_NORETURN;
		void catalog_loop() G_GNUC_NORETURN;

		/* Thread variables */
		mutable Glib::Threads::Mutex threads_mutex;
		mutable Glib::Threads::Cond threads_cond;
		Glib::Threads::Thread *threads_thread;
		std::map<gint64, std::shared_ptr<Thread>> threads;
		std::set<gint64> updatedThreads;
		void push_updated_thread(const gint64);
		void signal_404(const gint64 id);

		/* Catalog variables */
		mutable Glib::Threads::Mutex catalog_mutex;
		mutable Glib::Threads::Cond catalog_cond;
		Glib::Threads::Thread *catalog_thread;
		std::map<std::string, std::list<Glib::RefPtr<ThreadSummary> > > catalogs;
		std::set<std::string> boards;
		std::set<std::string> updated_boards;

		/* Curler is shared by both threads */
		Glib::Threads::Mutex curler_mutex;
		Curler curler;

	};

}
