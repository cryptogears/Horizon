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
#include <functional>
#include <glibmm/dispatcher.h>
#include <glibmm/object.h>
#include <glibmm/private/object_p.h>
#include <glibmm/class.h>

extern "C" {
#include "horizon_post.h"
}

namespace Horizon {
	class Post;

	class Post_Class : public Glib::Class {
	public:
		typedef Post CppObjectType;
		typedef HorizonPost BaseObjectType;
		typedef HorizonPostClass BaseClassType;
		typedef Glib::Object_Class CppClassParent;
		typedef GObjectClass BaseClassParent;

		friend class Post;

		const Glib::Class& init();
		static void class_init_function(void *g_class, void *class_data);
		static Glib::ObjectBase* wrap_new(GObject *object);
	};

	class Post : public Glib::Object {
	public:
		typedef Post CppObjectType;
		typedef Post_Class CppClassType;
		typedef HorizonPost BaseObjectType;
		typedef HorizonPostClass BaseObjectClassType;

	private: friend class Post_Class;
		static CppClassType post_class_;

	public:

		virtual ~Post();
		Post() = delete;
		explicit Post(const Post& in) = delete;
		Post& operator=(const Post& in) = delete;

		static GType get_type()   G_GNUC_CONST;
		static GType get_base_type()   G_GNUC_CONST;
		
		HorizonPost* gobj() {return reinterpret_cast<HorizonPost*>(gobject_);};
		const HorizonPost* gobj() const { return HORIZON_POST(gobject_);};
		HorizonPost* gobj_copy();

	protected:
		explicit Post(const Glib::ConstructParams& construct_params);
		explicit Post(HorizonPost* castitem);

	public:

		bool is_same_post(const Glib::RefPtr<Post> &post) const;
		bool is_not_same_post(const Glib::RefPtr<Post> &post) const;

		void update(const Post &in);
		void mark_rendered();
		bool is_rendered() const;

		std::string get_comment() const;
		gint64 get_id() const;
		gint64 get_unix_time() const;
		gint64 get_file_size() const;
		std::string get_subject() const;
		Glib::ustring get_time_str() const;
		std::string get_number() const;
		std::string get_name() const;
		std::string get_tripcode() const;
		std::string get_original_filename() const;
		std::string get_image_ext() const;
		std::string get_hash() const;
		std::string get_thumb_url();
		std::string get_image_url();
		gint get_thumb_width() const;
		gint get_thumb_height() const;
		gint get_width() const;
		gint get_height() const;
		std::size_t get_fsize() const;
		bool has_image() const;
		bool is_sticky() const;
		bool is_closed() const;
		bool is_deleted() const;
		bool is_spoiler() const;
		bool is_gif() const;
		void set_board(const std::string& board);
		std::string get_board() const;
		void set_thread_id(const gint64 id);
		gint64 get_thread_id() const;
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
		 Glib::TimeSpan get_update_interval() const;
		void update_notify(bool was_new);

		Glib::Dispatcher signal_updated_interval;

		/* Appends to the list any new posts
		   Marks changed posts (Thread lock/file deletion) as changed.
		 */
		void updatePosts(const std::list<Glib::RefPtr<Post> > &new_posts);
		const Glib::RefPtr<Post> get_first_post() const;

		/*
		 * Performs function func on each post in the threadview.  If
		 * Func ever returns true, this method will return
		 * true. Otherwise, it will return false.
		 */
		bool for_each_post(const std::function<bool  (const Glib::RefPtr<Post>&) >);
		bool should_notify() const;

	protected:
		Thread(std::string url);

	private:
		Thread() = delete;
		Thread(const Thread&) = delete;
		Thread& operator=(const Thread&) = delete;
		
		mutable Glib::Mutex posts_mutex;
		std::map<gint64, Glib::RefPtr<Post> > posts;

		std::vector<Glib::TimeSpan>::const_iterator update_interval_iter;
		//Glib::TimeSpan update_interval;
		//std::default_random_engine generator;
		//std::uniform_int_distribution<Glib::TimeSpan> random_int;
	};

	const Glib::TimeSpan MIN_UPDATE_INTERVAL = 10 * 1000 * 1000;
	const Glib::TimeSpan MAX_UPDATE_INTERVAL = 5 * 60 * 1000 * 1000;
	const Glib::TimeSpan NOTIFICATION_INTERVAL = 5 * 60 * 1000 * 1000;
	const std::vector<Glib::TimeSpan> UPDATE_INTERVALS = { 10, 10, 15, 18, 22, 25, 30, 45, 60, 90, 120, 180, 240, 300, 350, 450, 600 };

	void wrap_init();
		
}

namespace Glib
{
	Glib::RefPtr<Horizon::Post> wrap(HorizonPost* object, bool take_copy = false);
}
#endif
