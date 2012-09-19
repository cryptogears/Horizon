#include "post_view.hpp"

#include <iostream>
#include <glibmm/convert.h>
#include <gtkmm/viewport.h>
#include <gtkmm/scrolledwindow.h>
#include "horizon-resources.h"
#include <giomm/file.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>
#include <glibmm/markup.h>

namespace Horizon {

	PostView::PostView(const Post &in) :
		Gtk::Grid(),
		//textview(),
		comment(),
		comment_grid(),
		vadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		hadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		comment_viewport(hadjust, vadjust),
		post(in),
		ifetcher(ImageFetcher::get())
	{
		set_name("postview");
		set_orientation(Gtk::ORIENTATION_VERTICAL);
		
		auto ngrid = Gtk::manage(new Gtk::Grid());
		ngrid->set_name("posttopgrid");
		ngrid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		ngrid->set_column_spacing(5);

		auto label = Gtk::manage(new Gtk::Label(post.getSubject()));
		label->set_name("subject");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post.getName()));
		label->set_name("name");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post.getTimeStr()));
		label->set_name("time");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label("No. " + post.getNumber()));
		label->set_hexpand(false);
		label->set_justify(Gtk::JUSTIFY_LEFT);
		ngrid->add(*label);

		add(*ngrid);
		
		//ngrid = Gtk::manage(new Gtk::Grid());
		//ngrid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		//ngrid->set_name("comment_grid");
		
		//comment.set_name("posttextview");
		/*
		textview.set_name("posttextview");
		comment = textview.get_buffer();
		tag_table = comment->get_tag_table();

		//textview.set_wrap_mode(Gtk::WRAP_WORD);
		//textview.set_redraw_on_allocate(true);
		//textview.set_size_request(500, -1);
		//textview.set_editable(false);
		//textview.set_cursor_visible(false);
		//textview.set_hexpand(true);
		//		textview.set_hexpand_set(true);
		//		textview.set_hexpand(true);
		//textview.set_preferred_width(400,800);
		*/
		sax = (htmlSAXHandlerPtr) calloc(1, sizeof(xmlSAXHandler));
		sax->startElement = &startElement;
		sax->characters = &onCharacters;
		sax->serror = &on_xmlError;
		sax->initialized = XML_SAX2_MAGIC;

		built_string.clear();
		parseComment();

		comment.set_text(built_string);
		comment.set_valign(Gtk::ALIGN_START);
		comment.set_halign(Gtk::ALIGN_START);
		comment.set_justify(Gtk::JUSTIFY_LEFT);
		comment.set_line_wrap(true);
		comment.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
		//comment.set_size_request(400, -1);

		comment_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		comment_grid.add(comment);
		comment_viewport.set_name("commentview");
		comment_viewport.add(comment_grid);
		comment_viewport.set_hexpand(true);
		add(comment_viewport);

		if ( post.get_hash().size() > 0 ) {
			if ( ! ifetcher->has_thumb(post.get_hash()) ) {
				thumb_connection = ifetcher->signal_thumb_ready.connect(sigc::mem_fun(*this, &PostView::on_thumb_ready));
				ifetcher->download_thumb(post.get_hash(), post.get_thumb_url());
			} else {
				on_thumb_ready(post.get_hash());
			}
		}
	
		//comment.set_hexpand(true);
		//		comment.set_justify(Gtk::JUSTIFY_LEFT);
		//comment.set_halign(Gtk::ALIGN_START);
		//comment.set_valign(Gtk::ALIGN_START);
		/*
		comment->insert(comment->begin(), built_string);
		*/
		//auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/fang.png");
		//auto stream = file->read();
		//auto pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(stream, 150, 150, true);
		//comment->insert_pixbuf(comment->begin(), pixbuf);
		//auto image = new Gtk::Image(pixbuf);
		//image->set_name("image");
		//image->set_valign(Gtk::ALIGN_START);
		//image->set_alignment(0.0, 1.0);
		//auto anchor = comment->create_child_anchor(comment->begin());
		//textview.add_child_at_anchor(*image, anchor);
		//textview.add_child_in_window(*image, Gtk::TEXT_WINDOW_TEXT, 0, 0);
		//textview.set_left_margin(150);
		//		textview.set_border_window_size(Gtk::TEXT_WINDOW_LEFT, 150);
		//		textview.add_child_in_window(*image, Gtk::TEXT_WINDOW_LEFT, 0, 0);
		//Gtk::ScrolledWindow* vp = Gtk::manage(new Gtk::ScrolledWindow());
		//vp->add(textview);
		//vp->set_vexpand(false);
		//vp->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_NEVER);
		//textview.set_size_request(-1, 150);
		//ngrid->add(*image);
		//ngrid->add(textview);
		//vp->add(*label);
	}

	
	PostView::~PostView() {
		free(sax);
	}

	void PostView::refresh(const Post &in) {
		post = in;
	}

	void PostView::on_thumb_ready(std::string hash) {
		if ( post.get_hash().find(hash) != std::string::npos ) {
			if (thumb_connection.connected())
				thumb_connection.disconnect();
			try {
				auto image = new Gtk::Image(ifetcher->get_thumb(hash));
				Gtk::manage(image);
				
				image->set_halign(Gtk::ALIGN_START);
				image->set_valign(Gtk::ALIGN_START);
				int width = post.get_thumb_width();
				int height = post.get_thumb_height();
				if (width > 0 && height > 0) 
					;//image->set_size_request(post.get_thumb_width(), post.get_thumb_height());
				else
					g_warning("Thumb nail height is %d by %d", width, height);
				comment_grid.remove(comment);
				comment_grid.add(*image);
				comment_grid.add(comment);
				comment_grid.show_all();
			} catch ( Glib::Error e ) {
				g_warning("Error creating image from Pixmap: %s", e.what().c_str());
			}
		}
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
				//pv->comment_iter = pv->comment->insert(pv->comment_iter, "\n");
				pv->built_string.append("\n");
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

		pv->built_string.append(str);
		//pv->comment_iter = pv->comment->insert(pv->comment_iter, str);
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

