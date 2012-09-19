#include "thread.hpp"
#include <iostream>
#include <iomanip>

namespace Horizon {

	Post::Post(gpointer in, bool takeRef, std::string board_in):
		post(HORIZON_POST(in)),
		rendered(false),
		board(board_in)
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

	bool Post::operator==(const Post& rhs) {
		gint64 nol, nor;
		gint stickyl, stickyr, closedl, closedr;
		gint filedeletedl, filedeletedr;
		bool ret;
		g_object_get(post, 
		             "no", &nol,
		             "sticky", &stickyl,
		             "closed", &closedl,
		             "filedeleted", &filedeletedl,
		             NULL);
		g_object_get(rhs.post, 
		             "no", &nor,
		             "sticky", &stickyr,
		             "closed", &closedr,
		             "filedeleted", &filedeletedr,
		             NULL);
		ret = nor == nol 
			&& stickyr == stickyl 
			&& closedr == closedl 
			&& filedeletedr == filedeletedl;
		return ret;
	}

	void Post::update(const Post& in) {
		std::cout << "In Post::update!!!" << std::endl;
		g_object_unref(post);
		post = in.post;
		g_object_ref(post);
		rendered = false;
	}

	std::string Post::getComment() const {
		gchar* comment = NULL;
		g_object_get(post, "com", &comment, NULL);
		if (comment)
			return comment;
		else
			return "";
	}

	std::string Post::getSubject() const {
		gchar* comment = NULL;
		g_object_get(post, "sub", &comment, NULL);
		if (comment)
			return comment;
		else
			return "";
	}

	std::string Post::getTimeStr() const {
		gchar* comment = NULL;
		g_object_get(post, "now", &comment, NULL);
		if (comment)
			return comment;
		else
			return "";
	}

	std::string Post::getName() const {
		gchar* comment = NULL;
		g_object_get(post, "name", &comment, NULL);
		if (comment)
			return comment;
		else
			return "";
	}

	std::string Post::getNumber() const {
		gint64 id = 0;
		g_object_get(post, "no", &id, NULL);
		if (id > 1) {
			std::stringstream str;
			str << id;
			return str.str();
		}
		else
			return "";
	}

	const gint64 Post::getId() const {
		gint64 id = 0;
		g_object_get(post, "no", &id, NULL);
		return id;
	}

	const gint64 Post::getUnixTime() const {
		gint64 time = 0;
		g_object_get(post, "time", &time, NULL);
		return time;
	}

	std::string Post::get_hash() const {
		gchar *str = NULL;
		g_object_get(post, "md5", &str, NULL);
		if (str) {
			return str;
		} else {
			return "";
		}
	}

	std::string Post::get_thumb_url() const {
		gint64 tim = 0;
		std::stringstream ss;
		g_object_get(post, "tim", &tim, NULL);
		if (tim) {
			ss << "http://thumbs.4chan.org/";
			ss << board;
			ss << "/thumb/";
			ss << std::dec << tim;
			ss << "s.jpg";
			return ss.str();
		} else {
			return "";
		}
	}

const int Post::get_thumb_width() const {
	gint64 width = 0;
	g_object_get(post, "tn_w", &width, NULL);
	return static_cast<int>(width);
}
const int Post::get_thumb_height() const {
	gint64 height = 0;
	g_object_get(post, "tn_h", &height, NULL);
	return static_cast<int>(height);
}

	const bool Post::is_rendered() const {
		return rendered;
	}

	void Post::mark_rendered() {
		rendered = true;
	}

	Thread::Thread(std::string url) :
		full_url(url),
		last_post(0),
		last_checked(0),
		is_404(false)
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
			if ( posts.count(iter->getId()) == 0 ) {
				posts.insert({iter->getId(), *iter});
			} else {
				auto p = posts.find(iter->getId());
				if (p->second == *iter) {
				} else {
					std::cerr << ">Updating post " << iter->getId() << std::endl;
					p->second.update(*iter);
				}
			}
		}
	}

	std::shared_ptr<Thread> Thread::create(const std::string &url) {
		return std::shared_ptr<Thread>(new Thread(url));
	}

}
