#ifndef NATIVECHAN_THREAD_HPP
#define NATIVECHAN_THREAD_HPP
#include <string>
#include <list>
#include <glibmm/datetime.h>
#include <glib.h>
#include <glibmm/thread.h>
#include <memory>
#include <map>
#include <random>

extern "C" {
#include "horizon_post.h"
}

namespace Horizon {

	class Post {
	public:
		~Post();
		Post() = delete;
		explicit Post(gpointer in, bool takeRef, std::string board);
		explicit Post(HorizonPost* in, bool takeRef, std::string board);
		explicit Post(const Post& in);
		Post& operator=(const Post& in);

		const bool operator==(const Post& rhs) const;
		const bool operator!=(const Post& rhs) const;
		void update(const Post &in);
		void mark_rendered();
		const bool is_rendered() const;

		std::string get_comment() const;
		const gint64 get_id() const;
		const gint64 get_unix_time() const;
		std::string get_subject() const;
		Glib::ustring get_time_str() const;
		std::string get_number() const;
		std::string get_name() const;
		std::string get_original_filename() const;
		std::string get_image_ext() const;
		std::string get_hash() const;
		std::string get_thumb_url() const;
		const gint get_thumb_width() const;
		const gint get_thumb_height() const;
		const gint get_width() const;
		const gint get_height() const;
		const std::size_t get_fsize() const;
		const bool has_image() const;
		const bool is_sticky() const;
		const bool is_closed() const;
		const bool is_deleted() const;
		const bool is_spoiler() const;

	private:
		HorizonPost *post;
		std::string board;
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
		Glib::DateTime last_checked;
		Glib::DateTime last_post;
		bool is_404;
		const Glib::TimeSpan get_update_interval() const;
		void update_notify(bool was_new);

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

		Glib::TimeSpan update_interval;
		std::default_random_engine generator;
		std::uniform_int_distribution<Glib::TimeSpan> random_int;
	};

	const Glib::TimeSpan MIN_UPDATE_INTERVAL = 10 * 1000 * 1000;
	const Glib::TimeSpan MAX_UPDATE_INTERVAL = 15 * 60 * 1000 * 1000;
	const Glib::TimeSpan NOTIFICATION_INTERVAL = 5 * 60 * 1000 * 1000;

}
#endif
