#ifndef POST_VIEW_HPP
#define POST_VIEW_HPP
#include "thread.hpp"
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttagtable.h>
#include <gtkmm/textview.h>
#include <libxml/HTMLparser.h>

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
		
		htmlSAXHandlerPtr sax;
		xmlParserCtxtPtr ctxt;

		Post post;
		Gtk::TextView textview;
		Glib::RefPtr<Gtk::TextBuffer> comment;
		Glib::RefPtr<Gtk::TextTagTable> tag_table;
		Gtk::TextBuffer::iterator comment_iter;

		void on_start_element(const Glib::ustring &name, std::map<Glib::ustring, Glib::ustring> attr_map);
		void on_characters(const Glib::ustring&);
		friend void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
		friend void onCharacters(void* user_data, const xmlChar* chars, int len);
		friend void on_xmlError(void* user_data, xmlErrorPtr error);
		
		friend size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);

	};

	void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
	void onCharacters(void* user_data, const xmlChar* chars, int len);
	void on_xmlError(void* user_data, xmlErrorPtr error);
	size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);
}

#endif
