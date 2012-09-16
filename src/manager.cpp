#include "manager.hpp"
#include <iostream>

namespace Horizon {

	void Manager::addThread(std::shared_ptr<Thread> thread) {
		Glib::Mutex::Lock lock(data_mutex);
		
		threads.insert({thread->id, thread});
	}

	/* Run in a separate thread */
	void Manager::downloadThread(std::shared_ptr<Thread> thread) {
		Glib::Mutex::Lock lock(curler_mutex);

		try{
			std::list<Post> posts = curler.pullThread(thread->api_url,  thread->last_post);
			thread->last_checked = std::time(NULL);

			if (posts.size() > 0) {
				auto iter = posts.rbegin();
				thread->last_post = static_cast<std::time_t>(iter->getUnixTime());
				thread->updatePosts(posts);
				{
					Glib::Mutex::Lock data_lock(data_mutex);
					updatedThreads.insert(thread->id);
					signal_thread_updated();
				}
			}
		} catch (Thread404 e) {
			Glib::Mutex::Lock data_lock(data_mutex);
			thread->is_404 = true;
			updatedThreads.insert(thread->id);
			signal_thread_updated();
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
		std::time_t now = std::time(NULL);

		{
			Glib::Mutex::Lock lock(data_mutex);
			for(auto iter = threads.begin(); iter != threads.end(); iter++) {
				gint64 diff = now - iter->second->last_checked;
				if ( diff > MIN_UPDATE_INTERVAL && !iter->second->is_404 ) {
					threads_to_check.push_back(iter->second);
				}
			}
		}

		for ( auto iter = threads_to_check.begin(); 
		      iter != threads_to_check.end();
		      iter++) {
			Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Manager::downloadThread), *iter), false);
		}

		return true;
	}

}
