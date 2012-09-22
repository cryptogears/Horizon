#include "thread.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <glibmm/datetime.h>

namespace Horizon {

	Post::Post(gpointer in, bool takeRef, std::string board_in):
		post(HORIZON_POST(in)),
		board(board_in),
		rendered(false)
	{
		if (takeRef)
			g_object_ref(post);
		
		if (g_object_is_floating(post))
			g_object_ref_sink(post);
	}

	Post::Post(HorizonPost *in, bool takeRef, std::string board_in) :
		post(in),
		rendered(false),
		board(board_in)
	{

		if (takeRef)
			g_object_ref(post);

		if (g_object_is_floating(post))
			g_object_ref_sink(post);
	}

	Post::Post(const Post& in) {
		post = in.post;
		rendered = in.rendered;
		g_object_ref(post);
		board = in.board;
	}

	Post::~Post() {
		g_object_unref(post);
		post = NULL;
	}

	Post& Post::operator=(const Post& in) {
		g_object_unref(post);
		post = in.post;
		g_object_ref(post);

		rendered = in.rendered;

		return *this;
	}

	const bool Post::operator==(const Post& rhs) const {
		const gint64 nol        = get_id();
		const bool stickyl      = is_sticky();
		const bool closedl      = is_closed();
		const bool filedeletedl = is_deleted();

		const gint64 nor        = rhs.get_id();
		const bool stickyr      = rhs.is_sticky();
		const bool closedr      = rhs.is_closed();
		const bool filedeletedr = rhs.is_deleted();

		return nor == nol 
			&& stickyr == stickyl 
			&& closedr == closedl 
			&& filedeletedr == filedeletedl;
	}

	const bool Post::operator!=(const Post& rhs) const {
		const gint64 nol        = get_id();
		const bool stickyl      = is_sticky();
		const bool closedl      = is_closed();
		const bool filedeletedl = is_deleted();

		const gint64 nor        = rhs.get_id();
		const bool stickyr      = rhs.is_sticky();
		const bool closedr      = rhs.is_closed();
		const bool filedeletedr = rhs.is_deleted();

		return nor != nol 
			|| stickyr != stickyl 
			|| closedr != closedl 
			|| filedeletedr != filedeletedl;
	}

	void Post::update(const Post& in) {
		g_object_unref(post);
		post = in.post;
		g_object_ref(post);
		rendered = false;
	}

	std::string Post::get_comment() const {
		const gchar *comment = horizon_post_get_comment(post);
		std::stringstream out;

		if (comment) {
			out << comment;
		} 

		return out.str();
	}

	std::string Post::get_subject() const {
		const gchar *subject = horizon_post_get_subject(post);
		std::stringstream out;

		if (subject) {
			out << subject;
		}

		return out.str();
	}
		
	Glib::ustring Post::get_time_str() const {
		gint64 ctime = horizon_post_get_time(post);
		Glib::ustring out;
		Glib::DateTime time = Glib::DateTime::create_now_local(ctime);
		if (G_LIKELY( ctime >= 0 )) {
			out = time.format("%A, %-l:%M:%S %P");
		} 

		return out;
	}

	std::string Post::get_name() const {
		const gchar* name = horizon_post_get_name(post);
		std::stringstream out;

		if (name) {
			out << name;
		}
		
		return out.str();
	}

	std::string Post::get_number() const {
		gint64 id = horizon_post_get_post_number(post);
		std::stringstream out;

		if (id > 0) {
			out << id;
		}

		return out.str();
	}

	const gint64 Post::get_id() const {
		return horizon_post_get_post_number(post);
	}

	const gint64 Post::get_unix_time() const {
		return horizon_post_get_time(post);
	}

	std::string Post::get_hash() const {
		const gchar *str = horizon_post_get_md5(post);
		std::stringstream out;

		if (str) {
			out << str;
		} 

		return out.str();
	}

	std::string Post::get_thumb_url() const {
		gint64 tim = horizon_post_get_renamed_filename(post);
		std::stringstream out;

		if (tim > 0) {
			out << "http://thumbs.4chan.org/" << board << "/thumb/"
			    << std::dec << tim << "s.jpg";
		} 

		return out.str();
	}

	std::string Post::get_original_filename() const {
		const gchar *filename = horizon_post_get_original_filename(post);
		std::stringstream out;

		if (filename) {
			out << static_cast<const char*>(filename);
		}

		return out.str();
	}

	std::string Post::get_image_ext() const {
		const gchar *ext = horizon_post_get_ext(post);
		std::stringstream out;

		if (ext) {
			out << static_cast<const char*>(ext);
		}

		return out.str();
	}

	const gint Post::get_thumb_width() const {
		return horizon_post_get_thumbnail_width(post);
	}

	const gint Post::get_thumb_height() const {
		return horizon_post_get_thumbnail_height(post);
	}

	const gint Post::get_height() const {
		return horizon_post_get_height(post);
	}

	const gint Post::get_width() const {
		return horizon_post_get_width(post);
	}

	const std::size_t Post::get_fsize() const {
		return static_cast<const std::size_t>(horizon_post_get_fsize(post));
	}

	const bool Post::has_image() const {
		return horizon_post_get_fsize(post) > 0;
	}

	const bool Post::is_sticky() const {
		return static_cast<const bool>(horizon_post_get_sticky(post));
	}

	const bool Post::is_closed() const {
		return static_cast<const bool>(horizon_post_get_closed(post));
	}

	const bool Post::is_deleted() const {
		return static_cast<const bool>(horizon_post_get_deleted(post));
	}

	const bool Post::is_spoiler() const {
		return static_cast<const bool>(horizon_post_get_spoiler(post));
	}

	const bool Post::is_rendered() const {
		return rendered;
	}

	void Post::mark_rendered() {
		rendered = true;
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

	void Thread::updatePosts(const std::list<Post> &new_posts) {
		Glib::Mutex::Lock lock(posts_mutex);

		for ( auto iter = new_posts.begin(); iter != new_posts.end(); iter++ ) {
			if ( posts.count(iter->get_id()) == 0 ) {
				posts.insert({iter->get_id(), *iter});
			} else {
				auto p = posts.find(iter->get_id());
				if (p->second == *iter) {
				} else {
					p->second.update(*iter);
				}
			}
		}
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

	
}
