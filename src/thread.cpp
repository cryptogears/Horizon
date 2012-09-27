#include "thread.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <glibmm/datetime.h>

namespace Horizon {
	/*
	Post::Post(gpointer in, bool takeRef, const std::string &board_in) :
		post(HORIZON_POST(in))
	{
		if (takeRef)
			g_object_ref(post);
		
		if (g_object_is_floating(post))
			g_object_ref_sink(post);

		horizon_post_set_board(post, board_in.c_str());
	}

	Post::Post(HorizonPost *in, bool takeRef, const std::string &board_in) :
		post(in)
	{
		if (takeRef)
			g_object_ref(post);

		if (g_object_is_floating(post))
			g_object_ref_sink(post);

		horizon_post_set_board(post, board_in.c_str());
	}

	Post::Post(const Post& in) {
		post = in.post;
		g_object_ref(post);
	}

	Post& Post::operator=(const Post& in) {
		g_object_unref(post);
		post = in.post;
		g_object_ref(post);

		return *this;
	}
	*/

	const Glib::Class& Post_Class::init() {
		if (!gtype_) {
			class_init_func_ = &Post_Class::class_init_function;
			CppClassParent::CppObjectType::get_type();
			register_derived_type(horizon_post_get_type());
		}
		
		return *this;
	}

	void Post_Class::class_init_function(void *g_class, void *class_data) {
		BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
		CppClassParent::class_init_function(klass, class_data);
	}

	Glib::ObjectBase* Post_Class::wrap_new(GObject *object) {
		return new Post((HorizonPost*) object);
	}

	HorizonPost* Post::gobj_copy() {
		reference();
		return gobj();
	}

	Post::Post(HorizonPost* castitem) :
		Glib::Object((GObject*) castitem)
	{}

	Post::Post(const Glib::ConstructParams &params) :
		Glib::Object(params)
	{}

	Post::CppClassType Post::post_class_;

	GType Post::get_type() {
		return post_class_.init().get_type();
	}

	GType Post::get_base_type() {
		return horizon_post_get_type();
	}

	Post::~Post() {
	}

	void Post::set_board(const std::string& in) {
		horizon_post_set_board(const_cast<HorizonPost*>(gobj()), in.c_str());
	}

	const bool Post::is_same_post(const Glib::RefPtr<Post> &post) const {
		return horizon_post_is_same_post(const_cast<HorizonPost*>(gobj()), post->gobj());
	}

	const bool Post::is_not_same_post(const Glib::RefPtr<Post> &post) const {
		return horizon_post_is_not_same_post(const_cast<HorizonPost*>(gobj()), post->gobj());
	}

	void Post::update(const Post& in) {
		g_warning("Post update not implemented");
	}

	std::string Post::get_comment() const {
		const gchar *comment = horizon_post_get_comment(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (comment) {
			out << comment;
		} 

		return out.str();
	}

	std::string Post::get_subject() const {
		const gchar *subject = horizon_post_get_subject(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (subject) {
			out << subject;
		}

		return out.str();
	}
		
	Glib::ustring Post::get_time_str() const {
		gint64 ctime = horizon_post_get_time(const_cast<HorizonPost*>(gobj()));
		Glib::ustring out;
		Glib::DateTime time = Glib::DateTime::create_now_local(ctime);
		if (G_LIKELY( ctime >= 0 )) {
			out = time.format("%A, %-l:%M:%S %P");
		} 

		return out;
	}

	std::string Post::get_name() const {
		const gchar* name = horizon_post_get_name(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (name) {
			out << name;
		}
		
		return out.str();
	}

	std::string Post::get_number() const {
		gint64 id = horizon_post_get_post_number(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (id > 0) {
			out << id;
		}

		return out.str();
	}

	const gint64 Post::get_id() const {
		return horizon_post_get_post_number(const_cast<HorizonPost*>(gobj()));
	}

	const gint64 Post::get_unix_time() const {
		return horizon_post_get_time(const_cast<HorizonPost*>(gobj()));
	}

	std::string Post::get_hash() const {
		const gchar *str = horizon_post_get_md5(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (str) {
			out << str;
		} 

		return out.str();
	}

	std::string Post::get_thumb_url() const {
		const gchar *url = horizon_post_get_thumb_url(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (url) {
			out << url;
		} 

		return out.str();
	}

	std::string Post::get_image_url() const {
		const gchar *url = horizon_post_get_image_url(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;
		
		if (url) {
			out << url;
		}

		return out.str();
	}


	std::string Post::get_original_filename() const {
		const gchar *filename = horizon_post_get_original_filename(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (filename) {
			out << static_cast<const char*>(filename);
		}

		return out.str();
	}

	std::string Post::get_image_ext() const {
		const gchar *ext = horizon_post_get_ext(const_cast<HorizonPost*>(gobj()));
		std::stringstream out;

		if (ext) {
			out << static_cast<const char*>(ext);
		}

		return out.str();
	}

	const bool Post::is_gif() const {
		return static_cast<bool>(horizon_post_is_gif(const_cast<HorizonPost*>(gobj())));
	}

	const gint Post::get_thumb_width() const {
		return horizon_post_get_thumbnail_width(const_cast<HorizonPost*>(gobj()));
	}

	const gint Post::get_thumb_height() const {
		return horizon_post_get_thumbnail_height(const_cast<HorizonPost*>(gobj()));
	}

	const gint Post::get_height() const {
		return horizon_post_get_height(const_cast<HorizonPost*>(gobj()));
	}

	const gint Post::get_width() const {
		return horizon_post_get_width(const_cast<HorizonPost*>(gobj()));
	}

	const std::size_t Post::get_fsize() const {
		return static_cast<const std::size_t>(horizon_post_get_fsize(const_cast<HorizonPost*>(gobj())));
	}

	const bool Post::has_image() const {
		return horizon_post_has_image(const_cast<HorizonPost*>(gobj()));
	}

	const bool Post::is_sticky() const {
		return static_cast<const bool>(horizon_post_get_sticky(const_cast<HorizonPost*>(gobj())));
	}

	const bool Post::is_closed() const {
		return static_cast<const bool>(horizon_post_get_closed(const_cast<HorizonPost*>(gobj())));
	}

	const bool Post::is_deleted() const {
		return static_cast<const bool>(horizon_post_get_deleted(const_cast<HorizonPost*>(gobj())));
	}

	const bool Post::is_spoiler() const {
		return static_cast<const bool>(horizon_post_get_spoiler(const_cast<HorizonPost*>(gobj())));
	}

	const bool Post::is_rendered() const {
		return static_cast<const bool>(horizon_post_is_rendered(const_cast<HorizonPost*>(gobj())));
	}

	void Post::mark_rendered() {
		horizon_post_set_rendered(const_cast<HorizonPost*>(gobj()), true);
	}

	Thread::Thread(std::string url) :
		full_url(url),
		last_post(Glib::DateTime::create_now_utc(0)),
		last_checked(Glib::DateTime::create_now_utc(0)),
		is_404(false),
		update_interval(MIN_UPDATE_INTERVAL),
		generator(std::chrono::system_clock::now().time_since_epoch().count()),
		random_int(0, 13 * 1000 * 1000)
	{
		size_t res_pos = url.rfind("/res/");
		size_t board_pos = url.rfind("/", res_pos - 1);
		number = url.substr(res_pos + 5, std::string::npos);
		board = url.substr(board_pos + 1, res_pos - board_pos - 1 );
		api_url = "http://api.4chan.org/" + board + "/res/" + number + ".json";
		number = number.substr(0, number.find_first_of('#'));
		id = strtoll(number.c_str(), NULL, 10);
	}

	void Thread::updatePosts(const std::list<Glib::RefPtr<Post> > &new_posts) {
		Glib::Mutex::Lock lock(posts_mutex);

		for ( auto iter = new_posts.begin(); iter != new_posts.end(); iter++ ) {
			Glib::RefPtr<Post> post = *iter;
			auto conflicting_post = posts.find(post->get_id());
			if ( conflicting_post == posts.end() ) {
				posts.insert({post->get_id(), post});
			} else {
				if ( conflicting_post->second->is_not_same_post(post) ) {
					// The new post has updated metadata (sticky, file
					// deleted, or closed)
					posts.erase(conflicting_post);
					posts.insert({post->get_id(), post});
				}
			}
		}
	}

	const bool Thread::for_each_post(std::function<bool(const Glib::RefPtr<Post>&) > func) {
		Glib::Mutex::Lock lock(posts_mutex);
		bool ret = false;
		
		for ( auto iter : posts ) {
			bool thisret = func(iter.second);
			ret = ret || thisret;
		}

		return ret;
	}

	const Glib::RefPtr<Post> Thread::get_first_post() const {
		Glib::Mutex::Lock lock(posts_mutex);
		
		auto iter = posts.begin();
		return iter->second;
	}

	const Glib::TimeSpan Thread::get_update_interval() const { 
		return update_interval;
	}

	void Thread::update_notify(bool was_new) {
		if (G_UNLIKELY(was_new)) {
			update_interval = MIN_UPDATE_INTERVAL;
		} else {
			if (update_interval < MAX_UPDATE_INTERVAL) {
				update_interval = static_cast<Glib::TimeSpan>(update_interval + random_int(generator));
			} else { 
				update_interval = MAX_UPDATE_INTERVAL;
			}
		}

		signal_updated_interval();
	}

	std::shared_ptr<Thread> Thread::create(const std::string &url) {
		return std::shared_ptr<Thread>(new Thread(url));
	}

	void wrap_init() {
		Glib::wrap_register(horizon_post_get_type(), &Horizon::Post_Class::wrap_new);
		Horizon::Post::get_type();
	}
	
}

namespace Glib 
{
	Glib::RefPtr<Horizon::Post> wrap(HorizonPost *object, bool take_copy) {
		return Glib::RefPtr<Horizon::Post>( dynamic_cast<Horizon::Post*>( Glib::wrap_auto ((GObject*)object, take_copy)) );
	}
}
