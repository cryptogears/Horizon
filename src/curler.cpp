#include "curler.hpp"
#include <gio/gio.h>
#include <iostream>
#include <gio/gunixoutputstream.h>
#include <cstring>
#include "entities.h"
#include <json-glib/json-gobject.h>
#include "thread.hpp"


namespace Horizon {


	static size_t curler_thread_cb(char *ptr, size_t size, size_t nmemb, void* userdata)
	{
		Curler* curler = static_cast<Curler*>(userdata);
		curler->thread_writeback(ptr, size*nmemb);
		return size*nmemb;
	}

	Curler::Curler() :
		last_pull(Glib::DateTime::create_now_utc(0)),
		amWorking(false)
	{
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		parser = json_parser_new();
		Horizon::wrap_init();
	}

	Curler::~Curler() {
		g_object_unref(parser);
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}

	void Curler::thread_writeback(const void* data, gssize len) {
		threadBuffer.append(static_cast<const char*>(data), len);
	}

	std::list<Glib::RefPtr<Post> > Curler::pullThread(std::shared_ptr<Thread> thread) {
		CURLcode res;
		std::list<Glib::RefPtr<Post> > posts;
		Glib::DateTime completed;

		while ( Glib::DateTime::create_now_utc().difference(last_pull) < G_USEC_PER_SEC ) {
			g_usleep(G_USEC_PER_SEC);
		}

		if ( amWorking ) {
			throw *new Concurrency();
		}

		amWorking = true;
		threadBuffer.clear();
 
		res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		                       curler_thread_cb);
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA,
		                       static_cast<void*>(this));
		res = curl_easy_setopt(curl, CURLOPT_URL,
		                       thread->api_url.c_str());
		res = curl_easy_setopt(curl, CURLOPT_TIMECONDITION,
		                       CURL_TIMECOND_IFMODSINCE);
		res = curl_easy_setopt(curl, CURLOPT_TIMEVALUE,
		                       static_cast<long>(thread->last_post.to_unix()) + 1);
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE,
		                       0);

		last_pull = Glib::DateTime::create_now_utc();
		res = curl_easy_perform(curl);

		long code = 0;
		if (res != CURLE_OK) {
			switch (res) {
			case CURLE_HTTP_RETURNED_ERROR:
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
				if (code == 404) {
					amWorking = false;
					throw *new Thread404();
				} else {
					amWorking = false;
					throw *new CurlException(curl_easy_strerror(res));
				}
				break;
			default:
				amWorking = false;
				throw *new CurlException(curl_easy_strerror(res));
				break;
			}
		}

		completed = Glib::DateTime::create_now_utc();
		
		if ( threadBuffer.size() == 0 ) {
			thread->update_notify(false);
			amWorking = false;
			return posts;
		}


		GError *merror = NULL;
		if(json_parser_load_from_data(parser, threadBuffer.c_str(), threadBuffer.size(), &merror)) {
		} else {
			g_error("While parsing JSON: %s", merror->message);
		}


		JsonObject *jsonobject = json_node_get_object(json_parser_get_root(parser));
		JsonArray *array = json_node_get_array(json_object_get_member(jsonobject, "posts"));
		const guint num_posts = json_array_get_length(array);

		for ( guint i = 0; i < num_posts; i++ ) {
			
			JsonNode *obj = json_array_get_element(array, i);
			GObject *cpost =  json_gobject_deserialize( horizon_post_get_type(), obj );
			Glib::RefPtr<Post> post = Glib::wrap(HORIZON_POST(cpost));
			post->set_board(thread->board);
			post->set_thread_id(thread->id);
			//Post cpppost(cpost, false, thread->board);
			//Glib::RefPtr<Post> post = Glib::RefPtr<Post>(cpppost, false);
			posts.push_back(post);
		}

		amWorking = false;
		return posts;
	}

	std::list<Glib::RefPtr<ThreadSummary> > Curler::pullBoard(const std::string &url, const std::string &board) {
		CURLcode res;
		std::list<Glib::RefPtr<ThreadSummary> > summaries;
		
		while ( Glib::DateTime::create_now_utc().difference(last_pull) == 0 ) {
			g_usleep(G_USEC_PER_SEC);
		}

		threadBuffer.clear();

		res = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		                       curler_thread_cb);
		res = curl_easy_setopt(curl, CURLOPT_WRITEDATA,
		                       static_cast<void*>(this));
		res = curl_easy_setopt(curl, CURLOPT_URL,
		                       url.c_str());
		res = curl_easy_setopt(curl, CURLOPT_VERBOSE,
		                       0);

		last_pull = Glib::DateTime::create_now_utc();
		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			long code = 0;
			switch(res) {
			case CURLE_HTTP_RETURNED_ERROR:
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
				if (code == 404) {
					throw *new Thread404();
				} else {
					throw *new CurlException(curl_easy_strerror(res));
				}
				break;
			default:
				throw *new CurlException(curl_easy_strerror(res));
				break;
			}
		}

		if ( threadBuffer.size() == 0 ) {
			return summaries;
		}

		GError *merror = NULL;
		if (json_parser_load_from_data(parser, threadBuffer.c_str(), threadBuffer.size(), &merror)) {
		} else {
			g_error("While parsing JSON: %s", merror->message);
		}

		JsonObject *jsonobject = json_node_get_object(json_parser_get_root(parser));
		JsonObject *threads = json_node_get_object(json_object_get_member(jsonobject, "threads"));
		
		GList *list = json_object_get_members(threads);
		guint list_length = g_list_length(list);
		GList *first = g_list_first(list);

		for (int i = 0; i < list_length; i++) {
			if (first) {
				gpointer data = first->data;
				first = g_list_next(first);
				const gchar *member_name = static_cast<gchar*>(data);
				JsonNode *object = json_object_get_member(threads, member_name);
				GObject *csummary = json_gobject_deserialize( horizon_thread_summary_get_type(), object );
				Glib::RefPtr<ThreadSummary> summary = Glib::wrap(HORIZON_THREAD_SUMMARY(csummary));
				summary->set_id(member_name);
				summary->set_board(board.c_str());
				summaries.push_back(summary);
			}
		}
		
		    return summaries;
	}
}
