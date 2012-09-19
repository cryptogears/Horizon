#ifndef POST_VIEW_HPP
#define POST_VIEW_HPP
#include "thread.hpp"
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttagtable.h>
#include <gtkmm/textview.h>
#include <libxml/HTMLparser.h>
#include <gtkmm/viewport.h>
#include "image_fetcher.hpp"

namespace Horizon {

	class PostView : public Gtk::Grid {
	public:
		PostView( const Post &in );
		~PostView();

		void refresh( const Post &in );

	private:
		PostView() = delete;
		PostView(const PostView&) = delete;
		PostView& operator=(const PostView&) = delete;

		void parseComment();
		void on_thumb_ready(std::string hash);
		void on_image_ready(std::string hash);
		sigc::connection thumb_connection;

		htmlSAXHandlerPtr sax;
		xmlParserCtxtPtr ctxt;

		Post post;
		Glib::RefPtr<Gtk::Adjustment> hadjust, vadjust;
		Gtk::Grid comment_grid;
		Gtk::Label comment;
		Gtk::Viewport comment_viewport;

		std::shared_ptr<ImageFetcher> ifetcher;
		/*
		Gtk::TextView textview;
		Glib::RefPtr<Gtk::TextBuffer> comment;
		Glib::RefPtr<Gtk::TextTagTable> tag_table;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		*/
		Glib::ustring built_string;

		friend void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
		friend void onCharacters(void* user_data, const xmlChar* chars, int len);
		friend void on_xmlError(void* user_data, xmlErrorPtr error);
	};

	void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
	void onCharacters(void* user_data, const xmlChar* chars, int len);
	void on_xmlError(void* user_data, xmlErrorPtr error);
	size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);
}

#endif
