#include "manager.hpp"
#include <iostream>
#include <utility>

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

	void Manager::signal_404(const gint64 id) {
		Glib::Threads::Mutex::Lock lock(threads_mutex);

		auto iter = threads.find(id);
		if ( iter != threads.end() )
			threads.erase(iter);
	}

	/* Runs in a separate thread */
	void Manager::catalog_loop() {
		while (true) {
			bool is_new = false;
			std::list< std::pair<std::string, std::string> > pairs;
			{
				Glib::Threads::Mutex::Lock lock(catalog_mutex);
				for ( auto board : boards ) {
					std::stringstream url;
					url << "http://catalog.neet.tv/" << board << "/threads.json";
					pairs.push_back({url.str(), board});
				}
			}

			for (auto pair : pairs) {
				std::string url = pair.first;
				std::string board = pair.second;
				try {
					Glib::Threads::Mutex::Lock lock(curler_mutex);
					std::list<Glib::RefPtr<ThreadSummary> > new_summaries = curler.pullBoard(url, board);

					if (new_summaries.size() > 0) {
						Glib::Threads::Mutex::Lock lock(catalog_mutex);
						auto iter = catalogs.find(board);
						if (iter != catalogs.end()) {
							// TODO We want update the summary. For now, replace
							catalogs.erase(iter);
							catalogs.insert({board, new_summaries});
						} else {
							catalogs.insert({board, new_summaries});
						}
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
			{
				Glib::Threads::Mutex::Lock lock(catalog_mutex);
				catalog_cond.wait(catalog_mutex);
			}
		} // while true
	}

	/* Runs in a separate thread */
	void Manager::threads_loop() {
		while (true) {
			bool was_updated = false;
			// Build a list of threads that are past due for an update
			std::list<std::shared_ptr<Thread>> threads_to_check;
			Glib::DateTime now = Glib::DateTime::create_now_utc();
			
			{
				Glib::Threads::Mutex::Lock lock(threads_mutex);
				for(auto iter = threads.begin(); iter != threads.end(); iter++) {
					Glib::TimeSpan diff = now.difference(iter->second->last_checked);
					if ( diff > iter->second->get_update_interval() && !iter->second->is_404 ) {
						threads_to_check.push_back(iter->second);
					}
				}
			}


			for ( auto thread : threads_to_check ) {
				try{
					Glib::Threads::Mutex::Lock lock(curler_mutex);
					std::list<Glib::RefPtr<Post> > posts = curler.pullThread(thread);
					thread->last_checked = Glib::DateTime::create_now_utc();

					if (posts.size() > 0) {
						auto iter = posts.rbegin();
						thread->last_post = Glib::DateTime::create_now_utc((*iter)->get_unix_time());
						thread->updatePosts(posts);
						push_updated_thread(thread->id);
						was_updated = true;
					}
				} catch (Thread404 e) {
					thread->is_404 = true;
					push_updated_thread(thread->id);
					signal_thread_updated();
					signal_404(thread->id);
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
			{
				Glib::Threads::Mutex::Lock lock(threads_mutex);
				threads_cond.wait(threads_mutex);
			}
		} // while true
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
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		bool ret = false;
		for (auto board : boards) {
			ret = ret || func(board);
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

	const std::list<Glib::RefPtr<ThreadSummary> > Manager::get_catalog(const std::string &board) {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		auto iter = catalogs.find(board);
		if (iter != catalogs.end()) {
			return iter->second;
		} else {
			return std::list<Glib::RefPtr<ThreadSummary> >();
		}
	}

	static void launch_new_thread(Glib::Threads::Thread *&thread,
	                              sigc::slot<void> slot) {
		int trycount = 0;
		while ( thread == nullptr ) {
			try {
				trycount++;
				thread = Glib::Threads::Thread::create( slot );
			} catch (Glib::Threads::ThreadError e) {
				if ( e.code() == Glib::Threads::ThreadError::AGAIN ) {
					if (trycount > 20) {
						g_warning("Unable to create a new thread. Trycount: %d, Message: %s", trycount, e.what().c_str());
						break;
					}
				} else {
					g_warning("Unable to create a new thread. Trycount: %d Message: %s", trycount, e.what().c_str());
					break;
				}
			} 
		}
	}

	bool Manager::update_catalogs() {
		Glib::Threads::Mutex::Lock lock(catalog_mutex);
		int trycount = 0;
		if (G_UNLIKELY(catalog_thread == nullptr)) {
			launch_new_thread(catalog_thread, sigc::mem_fun(*this, &Manager::catalog_loop));
		}
		catalog_cond.signal();
		return true;
	}

	bool Manager::update_threads() {
		Glib::Threads::Mutex::Lock lock(threads_mutex);
		if (threads_thread == nullptr) {
			launch_new_thread(threads_thread, sigc::mem_fun(*this, &Manager::threads_loop));
		}

		threads_cond.signal();
		return true;
	}

	Manager::~Manager() {
	}
}
