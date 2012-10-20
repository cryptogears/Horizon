#include <glib.h>
#include <glibmm/threads.h>
#include <glibmm/dispatcher.h>
#include <memory>
#include <map>
#include <set>
#include "thread.hpp"
#include "curler.hpp"
#include "thread_summary.hpp"

#ifdef HAVE_EV___H
#include <ev++.h>
#else
#include <libev/ev++.h>
#endif

namespace Horizon {

	class Manager {
	public:
		Manager();
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
		/* Thread variables */
		mutable Glib::Threads::Mutex threads_mutex;
		std::map<gint64, std::shared_ptr<Thread>> threads;
		std::set<gint64> updatedThreads;
		void push_updated_thread(const gint64);
		void signal_404(const gint64 id);
		void check_threads();
		void check_catalogs();

		/* Catalog variables */
		mutable Glib::Threads::Mutex catalog_mutex;
		std::map<std::string, std::list<Glib::RefPtr<ThreadSummary> > > catalogs;
		std::set<std::string> boards;
		std::set<std::string> updated_boards;

		/* Curler is shared by both threads */
		mutable Glib::Threads::Mutex curler_mutex;
		Curler curler;

		Glib::Threads::Thread *ev_catalog_thread;
		Glib::Threads::Thread *ev_thread_thread;
		ev::dynamic_loop       ev_catalog_loop;
		ev::dynamic_loop       ev_thread_loop;

		ev::async               thread_queue_w;
		void                   on_thread_queue_w(ev::async &w, int);

		ev::async               catalog_queue_w;
		void                   on_catalog_queue_w(ev::async &w, int);

		ev::async               kill_thread_w;
		void                   on_kill_thread_w(ev::async &w, int);

		ev::async               kill_catalog_w;
		void                   on_kill_catalog_w(ev::async &w, int);
	};

}
