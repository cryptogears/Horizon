#include "manager.hpp"
#include <iostream>

namespace Horizon {

	void Manager::addThread(std::shared_ptr<Thread> thread) {
		Glib::Mutex::Lock lock(data_mutex);
		
		threads.insert({thread->id, thread});
	}

	void Manager::remove_thread(const gint64 id) {
		Glib::Mutex::Lock lock(data_mutex);
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
		Glib::Mutex::Lock lock(data_mutex);

		return !updatedThreads.empty();
	}

	gint64 Manager::pop_updated_thread() {
		Glib::Mutex::Lock lock(data_mutex);
		gint64 out = 0;
		
		if ( updatedThreads.size() > 0) {
			auto iter = updatedThreads.begin();
			out = *iter;

			updatedThreads.erase(iter);
		}

		return out;
	}

	void Manager::push_updated_thread(const gint64 id) {
		Glib::Mutex::Lock lock(data_mutex);

		if ( threads.count(id) > 0 ) 
			updatedThreads.insert(id);
	}

	void Manager::signal_404(const gint64 id) {
		Glib::Mutex::Lock lock(data_mutex);
		auto erased = threads.erase(id);
		if (G_UNLIKELY( erased != 1 )) {
			g_warning("Thread %d was marked as 404 but is not in manager's list. %d were erased.", id, erased);
		}
	}

	/* Runs in a separate thread */
	void Manager::downloadThread(std::shared_ptr<Thread> thread) {
		Glib::Mutex::Lock lock(curler_mutex);

		try{
			std::list<Post> posts = curler.pullThread(thread);
			thread->last_checked = Glib::DateTime::create_now_utc();

			if (posts.size() > 0) {
				auto iter = posts.rbegin();
				thread->last_post = Glib::DateTime::create_now_utc(iter->get_unix_time());
				thread->updatePosts(posts);
				push_updated_thread(thread->id);
				signal_thread_updated();
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

	}

	bool Manager::checkThreads() {
		// Build a list of threads that are past due for an update
		// Don't call downloadThread until we're done walking through
		// the threads map.
		std::list<std::shared_ptr<Thread>> threads_to_check;
		Glib::DateTime now = Glib::DateTime::create_now_utc();

		{
			Glib::Mutex::Lock lock(data_mutex);
			for(auto iter = threads.begin(); iter != threads.end(); iter++) {
				Glib::TimeSpan diff = now.difference(iter->second->last_checked);
				if ( diff > iter->second->get_update_interval() && !iter->second->is_404 ) {
					threads_to_check.push_back(iter->second);
				}
			}
		}

		for ( auto iter = threads_to_check.begin(); 
		      iter != threads_to_check.end();
		      iter++) {
			Glib::Threads::Thread* t = nullptr;
			int trycount = 0;
			while ( !t ) {
				try {
					trycount++;
					t = Glib::Threads::Thread::create( sigc::bind(sigc::mem_fun(*this, &Manager::downloadThread), *iter));
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

			if (t) {
				g_thread_unref(t->gobj());
			}
		}

		return true;
	}

	Manager::~Manager() {
	}
}
