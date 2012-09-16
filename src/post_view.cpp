#include "post_view.hpp"

#include <iostream>
#include <glibmm/convert.h>

namespace Horizon {

	PostView::PostView(const Post &in) :
		Grid(),
		textview(),
		post(in)
	{
		set_vexpand(true);
		set_hexpand(true);

		auto ngrid = Gtk::manage(new Gtk::Grid());
		ngrid->set_name("posttopgrid");
		ngrid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		ngrid->set_column_spacing(5);

		auto label = Gtk::manage(new Gtk::Label(post.getSubject()));
		label->set_name("subject");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post.getName()));
		label->set_name("name");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post.getTimeStr()));
		label->set_name("time");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label("No. " + post.getNumber()));
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(""));
		label->set_hexpand(true);
		ngrid->add(*label);

		attach(*ngrid,0,0,1,1);

		textview.set_name("posttextview");
		attach(textview, 0,1,1,1);
		set_orientation(Gtk::ORIENTATION_VERTICAL);
		comment = textview.get_buffer();
		tag_table = comment->get_tag_table();
		comment_iter = comment->begin();

		textview.set_wrap_mode(Gtk::WRAP_WORD);
		//textview.set_redraw_on_allocate(true);
		textview.set_size_request(500, -1);
		textview.set_editable(false);
		textview.set_cursor_visible(false);
		textview.set_vexpand(true);
		textview.set_hexpand(true);
		//		textview.set_hexpand_set(true);
		//		textview.set_hexpand(true);
		//		textview.set_preferred_width(400,800);

		sax = (htmlSAXHandlerPtr) calloc(1, sizeof(xmlSAXHandler));
		sax->startElement = &startElement;
		sax->characters = &onCharacters;
		sax->serror = &on_xmlError;
		sax->initialized = XML_SAX2_MAGIC;

		parseComment();
	}
	
	PostView::~PostView() {
		free(sax);
	}

	void PostView::refresh(const Post &in) {
		post = in;
	}

	void PostView::parseComment() {

		ctxt = htmlCreatePushParserCtxt(sax, this, NULL, 0, NULL,
		                                XML_CHAR_ENCODING_UTF8);
		if (G_UNLIKELY( ctxt == NULL )) {
			g_error("Unable to create libxml2 HTML context!");
		}

		htmlCtxtUseOptions(ctxt, HTML_PARSE_RECOVER );
		htmlParseChunk(ctxt, post.getComment().c_str(), post.getComment().size(), 1);
		htmlCtxtReset(ctxt);
		htmlFreeParserCtxt(ctxt);
		ctxt = NULL;
	}

	void startElement(void* user_data,
	                  const xmlChar* name,
	                  const xmlChar** attrs) {
		PostView* pv = static_cast<PostView*>(user_data);

		try {
			Glib::ustring str( reinterpret_cast<const char*>(name) );

			if ( str.find("br") != Glib::ustring::npos ) {
				pv->comment_iter = pv->comment->insert(pv->comment_iter, "\n");
			} else {
				if (false) {
					std::cerr << "Ignoring tag " << name;
					if (attrs != NULL) {
						std::cerr << " attrs:";
						for ( int i = 0; attrs[i] != NULL; i++) {
							std::cerr << " " << attrs[i];
					}
					}
					std::cerr << std::endl;
				}
			}
		} catch (std::exception e) {
			std::cerr << "Error startElement casting to string: " << e.what()
			          << std::endl;
		}
	}

	void onCharacters(void* user_data, const xmlChar* chars, int size) {
		PostView* pv = static_cast<PostView*>(user_data);

		Glib::ustring str;
		std::string locale;
		std::string locale_str;
		Glib::get_charset(locale);
		
		try {
			// TODO: This is way too much work; xmlChar is already in
			// UTF-8, we should be able to stick it right in a
			// ustring. Only problem is everytime I try it barfs.
			locale_str = Glib::convert(std::string(reinterpret_cast<const char*>(chars), size), locale, "UTF-8");
			str = Glib::locale_to_utf8(locale_str);
			
		} catch (Glib::ConvertError e) {
			g_error("Error casting '%s' to Glib::ustring: %s", chars, e.what().c_str());
		}

		pv->comment_iter = pv->comment->insert(pv->comment_iter, str);
	}

	size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data) {

	}

	void on_xmlError(void* user_data, xmlErrorPtr error) {
		Horizon::PostView* postview = static_cast<PostView*>(user_data);
		switch (error->code) {
		case XML_ERR_INVALID_ENCODING:
			std::cerr << "Warning: Invalid UTF-8 encoding. I blame moot. "
			          << "If you're saving by original filename, its going to get"
			          << " ugly. It can't be helped." << std::endl;
			break;
		case XML_ERR_NAME_REQUIRED:
		case XML_ERR_TAG_NAME_MISMATCH:
		case XML_ERR_ENTITYREF_SEMICOL_MISSING:
		case XML_ERR_INVALID_DEC_CHARREF:
		case XML_ERR_LTSLASH_REQUIRED:
		case XML_ERR_GT_REQUIRED:
			// Ignore
			break;
		default:
			if (error->domain == XML_FROM_HTML) {
				std::cerr << "Warning: Got unexpected libxml2 HTML error code "
				          << error->code << ". This is probably ok : " 
				          << error->message;
			} else {
				std::cerr << "Error: Got unexpected libxml2 error code "
				          << error->code << " from domain " << error->domain
				          << " which means: " << error->message << std::endl;
				std::cerr << "\tPlease report this error to the developer."
				          << std::endl;
			}
		}
	}


}

