#ifndef NATIVECHAN_THREAD_HPP
#define NATIVECHAN_THREAD_HPP
#include <string>
#include <list>
#include <ctime>
#include <glib.h>
#include <glibmm/thread.h>
#include <memory>
#include <map>

extern "C" {
#include "horizon_post.h"
}

namespace Horizon {

	class Post {
	public:
		~Post();
		Post() = delete;
		explicit Post(gpointer in, bool takeRef);
		explicit Post(HorizonPost* in, bool takeRef);
		explicit Post(const Post& in);
		Post& operator=(const Post& in);

		bool operator==(const Post& rhs);
		void update(const Post &in);
		void mark_rendered();
		const bool is_rendered() const;

		std::string getComment() const;
		const gint64 getId() const;
		const gint64 getUnixTime() const;
		std::string getSubject() const;
		std::string getTimeStr() const;
		std::string getNumber() const;
		std::string getName() const;

		
	private:
		HorizonPost *post;

		bool rendered;
	};

	class Thread {
	public:
		static std::shared_ptr<Thread> create(const std::string &url);

		gint64 id;
		std::string number;
		const std::string full_url;
		std::string api_url;
		std::string board;
		std::time_t last_checked;
		std::time_t last_post;
		bool is_404;

		/* Appends to the list any new posts
		   Marks changed posts (Thread lock/file deletion) as changed.
		 */
		void updatePosts(const std::list<Post> &new_posts);
		std::map<gint64, Post> posts;

	protected:
		Thread(std::string url);

	private:
		Thread() = delete;
		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;
		
		Glib::Mutex posts_mutex;
	};


}
#endif
