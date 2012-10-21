#include "manager.hpp"
#include <iostream>
#include <utility>
#include "utils.hpp"

namespace Horizon {

	void Manager::add_thread(std::shared_ptr<Thread> thread) {
		Glib::Threads::Mutex::Lock lock(threads_mutex);
		
		threads.insert({thread->id, thread});
	}

	void Manager::remove_thread(const gint64 id) {
		Glib::Threads::Mutex::Lock lock(threads_mutex);
		auto iter = threads.find(id);
		if ( iter != threads.end() ) {
			threads.erase(iter);
		}

		auto iter2 = updatedThreads.find(id);
		if ( iter2 != updatedThreads.end() ) {
			updatedThreads.erase(iter2);
		}
	}

	bool Manager::is_updated_thread() const {
		Glib::Threads::Mutex::Lock lock(threads_mutex);

		return !updatedThreads.empty();
	}

	gint64 Manager::pop_updated_thread() {
		Glib::Threads::Mutex::Lock lock(threads_mutex);
		gint64 out = 0;
		
		if ( updatedThreads.size() > 0) {
			auto iter = updatedThreads.begin();
			out = *iter;

			updatedThreads.erase(iter);
		}

		return out;
	}

	void Manager::push_updated_thread(const gint64 id) {
		Glib::Threads::Mutex::Lock lock(threads_mutex);

		if ( threads.count(id) > 0 ) 
			updatedThreads.insert(id);
	}

	void Manager::on_404(const gint64 id) {
		Glib::Threads::Mutex::Lock lock(threads_mutex);

		auto iter = threads.find(id);
		if ( iter != threads.end() )
			threads.erase(iter);
	}

	/* Runs in a separate thread */
	void Manager::check_catalogs() {
		bool is_new = false;
		std::vector< std::string > work_list;
		{
			Glib::Threads::Mutex::Lock lock(catalog_mutex);
			work_list.reserve(boards.size());
			std::copy(boards.begin(),
			          boards.end(),
			          std::back_inserter(work_list));
		}

		for (auto board : work_list) {
			std::stringstream url_stream;
			url_stream << "http://catalog.neet.tv/" << board << "/threads.json";
			std::string url = url_stream.str();

			try {
				Glib::Threads::Mutex::Lock lock(curler_mutex);
				std::list<Glib::RefPtr<ThreadSummary> > new_summaries = curler.pullBoard(url, board);

				if (new_summaries.size() > 0) {
					Glib::Threads::Mutex::Lock lock(catalog_mutex);
					auto iter = catalogs.find(board);
					if (iter != catalogs.end()) {
						// TODO We want update the summary. For now, replace
						catalogs.erase(iter);
					}
					catalogs.insert({board, new_summaries});

					updated_boards.insert(board);
					is_new = true;
				}
			} catch (Thread404 e) {
				std::cerr << "Got 404 while trying to pull catalog from "
				          <<  url << std::endl;
			} catch (CurlException e) {
				std::cerr << "Error: While pulling the catalog for "
				          << "/" << board << "/ :"
				          << e.what() << std::endl;
			}
		} // for boards
		if (is_new)
			signal_catalog_updated();
	}

	/* Runs in a separate thread */
	void Manager::check_threads() {
		// Build a list of threads that are past due for an update
		std::vector<std::pair<gint64, std::shared_ptr<Thread> > > threads_to_check;
		Glib::DateTime now = Glib::DateTime::create_now_utc();
			
		{
			Glib::Threads::Mutex::Lock lock(threads_mutex);
			threads_to_check.reserve(threads.size());
			std::copy_if(threads.begin(),
			             threads.end(),
			             std::back_inserter(threads_to_check),
			             [&now](std::pair<gint64, std::shared_ptr<Thread> > pair) {
				             std::shared_ptr<Thread> t = pair.second;
				             Glib::TimeSpan diff = std::abs(now.difference(t->last_checked));
				             return ( diff > t->get_update_interval() &&
				                      !t->is_404 );
			             });
		}

		for ( auto pair : threads_to_check ) {
			auto thread = pair.second;
			try{
				Glib::Threads::Mutex::Lock lock(curler_mutex);
				std::list<Glib::RefPtr<Post> > posts = curler.pullThread(thread);
				thread->last_checked = Glib::DateTime::create_now_utc();

				if (posts.size() > 0) {
					auto iter = posts.rbegin();
					thread->last_post = Glib::DateTime::create_now_utc((*iter)->get_unix_time());
					thread->updatePosts(posts);
					push_updated_thread(thread->id);
				}
			} catch (Thread404 e) {
				thread->is_404 = true;
				push_updated_thread(thread->id);
				on_404(thread->id);
			} catch (Concurrency e) {
				// This should be impossible since we are using the mutex.
				g_critical("Manager's Curler is being used by more than one thread.");
			} catch (TooFast e) {
				g_warning("Attempted to hit the 4chan JSON API faster than once a second.");
			} catch (CurlException e) {
				g_warning("Got Curl error: %s", e.what());
			}
		} // for threads_to_check

		signal_thread_updated();
	}

	/*
	 * Adds a catalog board to list of boards to monitor. Board should
	 * be akin to "a".
	 * Return value: true if the inserted board was a new board.
	 */
	bool Manager::add_catalog_board(const std::string &board) {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		auto pair = boards.insert(board);
		return pair.second;
	}

	/*
	 * Removes a catalog board from list of boards to check.
	 * Board should be akin to "a".
	 * Return value: true if the board was being monitored.
	 */
	bool Manager::remove_catalog_board(const std::string &board) {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		auto iter = boards.find(board);
		if (iter == boards.end()) {
			return false;
		} else {
			boards.erase(iter);
			return true;
		}
	}

	bool Manager::for_each_catalog_board(std::function<bool (const std::string&)> func) const {
		std::set<std::string> boards_copy;
		{
			Glib::Threads::Mutex::Lock lock(catalog_mutex);
			boards_copy = boards;
		}

		bool ret = false;
		for (auto board : boards_copy) {
			bool b = func(board);
			ret = ret || b;
		}

		return ret;
	}

	bool Manager::is_updated_catalog() const {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		return updated_boards.size() > 0;
	}

	const std::string Manager::pop_updated_catalog_board() {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		if (updated_boards.size() > 0) {
			auto iter = updated_boards.begin();
			const std::string out = *iter;
			updated_boards.erase(iter);
			return out;
		} else {
			return "";
		}
	}

	const std::list<Glib::RefPtr<ThreadSummary> >& Manager::get_catalog(const std::string &board) const {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		auto iter = catalogs.find(board);
		if (iter != catalogs.end()) {
			return iter->second;
		} else {
			throw std::range_error("Catalog is empty");
		}
	}

	bool Manager::update_catalogs() {
		catalog_queue_w.send();

		return true;
	}

	bool Manager::update_threads() {
		thread_queue_w.send();

		return true;
	}

	void Manager::on_catalog_queue_w(ev::async &, int) {
		check_catalogs();
	}

	void Manager::on_thread_queue_w(ev::async &, int) {
		check_threads();
	}

	void Manager::on_kill_thread_w(ev::async &, int) {
		thread_queue_w.stop();
		kill_thread_w.stop();
	}

	void Manager::on_kill_catalog_w(ev::async &, int) {
		catalog_queue_w.stop();
		kill_catalog_w.stop();
	}

	Manager::Manager() :
		ev_catalog_thread(nullptr),
		ev_thread_thread(nullptr),
		ev_catalog_loop(ev::AUTO),
		ev_thread_loop(ev::AUTO),
		thread_queue_w(ev_thread_loop),
		catalog_queue_w(ev_catalog_loop),
		kill_thread_w(ev_thread_loop),
		kill_catalog_w(ev_catalog_loop)
	{
		thread_queue_w.set<Manager, &Manager::on_thread_queue_w> (this);
		catalog_queue_w.set<Manager, &Manager::on_catalog_queue_w> (this);
		kill_thread_w.set<Manager, &Manager::on_kill_thread_w> (this);
		kill_catalog_w.set<Manager, &Manager::on_kill_catalog_w> (this);
		thread_queue_w.start();
		catalog_queue_w.start();
		kill_thread_w.start();
		kill_catalog_w.start();

		const sigc::slot<void> catalog_slot = sigc::bind(sigc::mem_fun(ev_catalog_loop, &ev::dynamic_loop::run), 0);
		const sigc::slot<void> thread_slot = sigc::bind(sigc::mem_fun(ev_thread_loop, &ev::dynamic_loop::run), 0);
		int trycount = 0;

		while ( ev_catalog_thread == nullptr && trycount++ < 10 ) {
			try {
				ev_catalog_thread = Horizon::create_named_thread("Catalog Curler",
				                                                 catalog_slot);
			} catch ( Glib::Threads::ThreadError e) {
				if (e.code() != Glib::Threads::ThreadError::AGAIN) {
					g_error("Couldn't create ImageFetcher thread: %s",
					        e.what().c_str());
				}
			}
		}

		trycount = 0;
		while ( ev_thread_thread == nullptr && trycount++ < 10 ) {
			try {
				ev_thread_thread = Horizon::create_named_thread("Thread Curler",
				                                                thread_slot);
			} catch ( Glib::Threads::ThreadError e) {
				if (e.code() != Glib::Threads::ThreadError::AGAIN) {
					g_error("Couldn't create ImageFetcher thread: %s",
					        e.what().c_str());
				}
			}
		}

		if (!ev_catalog_thread)
			g_error("Couldn't spin up Catalog Thread");
		if (!ev_thread_thread)
			g_error("Couldn't spin up Thread Thread");

	}

	Manager::~Manager() {
		kill_catalog_w.send();
		kill_thread_w.send();
		ev_catalog_thread->join();
		ev_thread_thread->join();
	}
}
