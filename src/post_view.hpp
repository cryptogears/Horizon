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
#include <giomm/dbusproxy.h>
#include "image_fetcher.hpp"

namespace Horizon {

	class PostView : public Gtk::Grid {
	public:
		PostView( const Post &in, const bool notify);
		~PostView();

		void refresh( const Post &in );
		const bool should_notify() const;
		const bool should_notify(const bool notify);

	private:
		PostView() = delete;
		PostView(const PostView&) = delete;
		PostView& operator=(const PostView&) = delete;

		static Glib::RefPtr<Gio::DBus::Proxy> get_dbus_proxy();
		void do_notify(const Glib::RefPtr<Gdk::Pixbuf>& = Glib::RefPtr<Gdk::Pixbuf>());


		void parseComment();
		void on_thumb_ready(std::string hash);
		void on_image_ready(std::string hash);
		sigc::connection thumb_connection;

		bool notify;

		htmlSAXHandlerPtr sax;
		xmlParserCtxtPtr ctxt;

		Post post;
		Glib::RefPtr<Gtk::Adjustment> hadjust, vadjust;
		Gtk::Grid comment_grid;
		Gtk::Label comment;
		Gtk::Viewport comment_viewport;
		void on_postview_show();
		void on_style_change();
		int derp = 0;

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
