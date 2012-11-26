#include "post_view.hpp"

#include <iterator>
#include <iostream>
#include <gtkmm/viewport.h>
#include <gtkmm/scrolledwindow.h>
#include "horizon-resources.h"
#include <giomm/file.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>
#include <glibmm/markup.h>
#include <glibmm/datetime.h>
#include <gtkmm/socket.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/comboboxtext.h>
#include "html_parser.hpp"
#include "horizon_image.hpp"
#include <giomm/unixoutputstream.h>
#include <glibmm/fileutils.h>
#include <glibmm/spawn.h>
#include <gtksourceviewmm.h>

namespace Horizon {

	PostView::PostView(const Glib::RefPtr<Post> &in) :
		Gtk::Grid(),
		post(in),
		post_info_grid (Gtk::manage(new Gtk::Grid())),
		image_info_grid(Gtk::manage(new Gtk::Grid())),
		content_grid   (Gtk::manage(new Gtk::Grid())),
		viewport_grid  (Gtk::manage(new Gtk::Grid())),
		comment_box    (Gtk::manage(new Gtk::Viewport(Gtk::Adjustment::create(0,0,0), Gtk::Adjustment::create(0,0,0)))),
		comment        (Gtk::manage(new Gtk::Label())),
		linkbacks      (Gtk::manage(new Gtk::Label())),
		image          (nullptr)
	{
		set_name("postview");
		set_orientation(Gtk::ORIENTATION_VERTICAL);

		viewport_grid->set_orientation(Gtk::ORIENTATION_VERTICAL);
		
		post_info_grid->set_name("posttopgrid");
		post_info_grid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		post_info_grid->set_column_spacing(5);

		auto label = Gtk::manage(new Gtk::Label(post->get_subject()));
		label->set_name("subject");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		post_info_grid->add(*label);

		Glib::ustring capcode = post->get_capcode();
		Glib::ustring name = post->get_name();
		Glib::ustring tripcode = post->get_tripcode();
		if (capcode.size() > 0) {
			capcode = Glib::ustring::compose("%1%2",
			                                 capcode.substr(0, 1).uppercase(),
			                                 capcode.substr(1));
			Glib::ustring namecap = Glib::ustring::compose("%1%2 ## %3",
			                                               name,
			                                               tripcode,
			                                               capcode);
			label = Gtk::manage(new Gtk::Label(namecap));
			label->set_name("capcode_" + post->get_capcode());
		} else {
			label = Gtk::manage(new Gtk::Label(name));
			label->set_name("name");
			label->set_ellipsize(Pango::ELLIPSIZE_END);
		}
		label->set_hexpand(false);
		post_info_grid->add(*label);

		if (capcode.size() == 0 &&
		    tripcode.size() > 0) {
			label = Gtk::manage(new Gtk::Label(post->get_tripcode()));
			label->set_name("tripcode");
			label->set_hexpand(false);
			label->set_ellipsize(Pango::ELLIPSIZE_END);
			post_info_grid->add(*label);
		}


		label = Gtk::manage(new Gtk::Label(post->get_time_str()));
		label->set_name("time");
		label->set_hexpand(false);
		post_info_grid->add(*label);

		label = Gtk::manage(new Gtk::Label("No. " + post->get_number()));
		label->set_hexpand(false);

		label->set_justify(Gtk::JUSTIFY_LEFT);
		post_info_grid->add(*label);

		viewport_grid->add(*post_info_grid);
		
		// Image info
		if ( post->has_image() ) {
			image_info_grid->set_name("imageinfogrid");
			image_info_grid->set_column_spacing(5);

			std::string filename = post->get_original_filename() + post->get_image_ext();

			label = Gtk::manage(new Gtk::Label(filename));
			label->set_name("originalfilename");
			label->set_hexpand(false);
			label->set_ellipsize(Pango::ELLIPSIZE_END);
			image_info_grid->add(*label);

			const gint64 original_id = g_ascii_strtoll(post->get_original_filename().c_str(), NULL, 10);
			const gint64 original_time  = original_id / 1000;
			if (original_time > 1000000000 && original_time < 10000000000) {
				Glib::DateTime dtime = Glib::DateTime::create_now_local(original_time);
				label = Gtk::manage(new Gtk::Label(dtime.format("%B %-e, %Y")));
				label->set_hexpand(false);
				label->set_name("imagenameinfo");
				image_info_grid->add(*label);
			} 

			std::stringstream imgdims;
			imgdims << " (" << post->get_fsize() / 1000 
			        << " KB, " << post->get_width() 
			        << "x" << post->get_height() << ") ";

			label = Gtk::manage(new Gtk::Label(imgdims.str()));
			label->set_hexpand(false);
			label->set_justify(Gtk::JUSTIFY_LEFT);
			label->set_name("imagenameinfo");
			image_info_grid->add(*label);
			viewport_grid->add(*image_info_grid);
		}

		linkbacks->signal_activate_link().connect(sigc::mem_fun(*this, &PostView::on_activate_link), false);
		linkbacks->set_justify(Gtk::JUSTIFY_LEFT);
		linkbacks->set_halign(Gtk::ALIGN_START);
		linkbacks->set_line_wrap(true);
		linkbacks->set_line_wrap(Pango::WRAP_WORD_CHAR);

		comment->set_name("comment");
		comment->set_selectable(true);
		comment->set_valign(Gtk::ALIGN_START);
		comment->set_halign(Gtk::ALIGN_START);
		comment->set_justify(Gtk::JUSTIFY_LEFT);
		comment->set_line_wrap(true);
		comment->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
		comment->set_track_visited_links(true);
		comment->signal_activate_link().connect(sigc::mem_fun(*this, &PostView::on_activate_link), false);


		content_grid->set_name("commentgrid");
		content_grid->add(*comment);

		viewport_grid->add(*content_grid);
		viewport_grid->set_hexpand(false);
		
		comment_box->set_name("commentview");
		comment_box->add(*viewport_grid);
		comment_box->set_hexpand(false);
		add(*comment_box);
		set_hexpand(true);

		set_comment_grid();

		if ( post->has_image() ) {
			auto s = sigc::mem_fun(*this, &PostView::on_image_state_changed);
			image = Gtk::manage(new Image(post, s));
			image->set_hexpand(false);
		}
	}

	PostView::~PostView() {
	}

	Glib::RefPtr<Post> PostView::get_post() const {
		return post;
	}

	Glib::ustring PostView::get_comment_body() const {
		return comment->get_text();
	}

	Glib::RefPtr<Gdk::Pixbuf> PostView::get_image() const {
		if (image)
			return image->get_image();
		else
			return Glib::RefPtr<Gdk::Pixbuf>();
	}

	static Glib::ustring str_squish(const Glib::ustring& in) {
		if (in.size() > 0) {
			auto start =   std::begin(in);
			auto end   = --std::end  (in);
			while ( start != in.end()   && *start == '\n' )
				++start;
			while ( end   != in.begin() && *end   == '\n' )
				--end;
			if ( start <= end ) {
				return Glib::ustring(start, ++end);
			}
		}

		return Glib::ustring();
	}

	static void on_language_changed(Glib::RefPtr<Gsv::Buffer>& buf, const Gtk::ComboBoxText* cbox) {
		auto const lmgr = Gsv::LanguageManager::get_default();
		buf->set_language(lmgr->get_language(cbox->get_active_text()));
	}

	void PostView::launch_editor(const Glib::ustring& code_text) {
		std::string filename;
		std::stringstream prefix;
		prefix << "Horizon_" << post->get_id() << "_";
		int fd = Glib::file_open_tmp(filename, prefix.str());
		auto ostream_ptr = Gio::UnixOutputStream::create(fd, true);
		ostream_ptr->write(code_text);
		ostream_ptr->close();
		std::stringstream cmdline;
		cmdline << "gvim " << filename;
		Glib::spawn_command_line_async(cmdline.str());
	}

	void PostView::set_comment_grid() {
		auto parser  = HtmlParser::getHtmlParser();
		auto strings = parser->html_to_pango(post->get_comment(), post->get_thread_id());
		comment->set_markup(str_squish(strings.front()));
		if (strings.size() > 1) {
			bool is_code = true;
			auto iter = ++strings.begin();
			for ( ; iter != strings.end(); ++iter) {
				if (is_code) {
					auto const code_text = str_squish(*iter);
					auto const lmgr      = Gsv::LanguageManager::get_default();	
					auto const ids       = lmgr->get_language_ids();
					auto buf    = Gsv::Buffer::create();
					auto grid   = Gtk::manage(new Gtk::Grid());
					auto cbox   = Gtk::manage(new Gtk::ComboBoxText(true));
					auto btn    = Gtk::manage(new Gtk::Button("Launch Editor"));
					auto label  = Gtk::manage(new Gtk::Label(""));
					auto sview  = Gtk::manage(new Gsv::View(buf));
					label->set_hexpand(true);
					btn  ->set_hexpand(false);
					cbox ->set_hexpand(false);
					buf->set_text(code_text);
					buf->set_language(lmgr->get_language("c"));
					typedef void (Gtk::ComboBoxText::*MethodType)(const Glib::ustring&);
					std::for_each(std::begin(ids),
					              std::end(ids),
					              std::bind(MethodType(&Gtk::ComboBoxText::append),
					                        cbox, std::placeholders::_1));
					cbox->set_active_text("c");
					auto on_language = sigc::bind(sigc::ptr_fun(&on_language_changed),
					                              buf, cbox);
					auto on_launch   = sigc::bind(sigc::mem_fun(*this, &PostView::launch_editor),
					                              code_text);
					btn ->signal_clicked().connect(on_launch);
					cbox->signal_changed().connect(on_language);

					grid->attach(*cbox,  0, 0, 1, 1);
					grid->attach(*label, 1, 0, 1, 1);
					grid->attach(*btn,   2, 0, 1, 1);
					grid->attach(*sview, 0, 1, 3, 1);
					viewport_grid->add(*grid);
				} else {
					Gtk::Label* c = Gtk::manage(new Gtk::Label());
					c->set_markup(str_squish(*iter));
					c->set_selectable(true);
					c->set_valign(Gtk::ALIGN_START);
					c->set_halign(Gtk::ALIGN_START);
					c->set_justify(Gtk::JUSTIFY_LEFT);
					c->set_line_wrap(true);
					c->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
					c->set_track_visited_links(true);
					c->signal_activate_link().connect(sigc::mem_fun(*this, &PostView::on_activate_link), false);
					viewport_grid->add(*c);
				}
				is_code = !is_code;
			}
		}
	}

	void PostView::set_image_state(const Image::ImageState new_state) {
		if (image)
			image->set_state(new_state);
	}

	void PostView::on_image_state_changed(const Image::ImageState &state) {
		switch (state) {
		case Image::NONE:
			if (image->get_parent() != nullptr) {
				content_grid->remove(*image);
			}
			image->set_hexpand(false);
			break;
		case Image::THUMBNAIL:
			content_grid->remove(*comment);
			if (image->get_parent() == nullptr) {
				content_grid->add(*image);
			}
			content_grid->attach_next_to(*comment, *image, Gtk::POS_RIGHT, 1, 1);
			image->set_hexpand(false);
			break;
		case Image::FULL:
		case Image::EXPAND:
			content_grid->remove(*comment);
			content_grid->attach_next_to(*comment, *image, Gtk::POS_BOTTOM, 1, 1);
			image->set_hexpand(true);
			break;
		}
		queue_resize();
	}

	void PostView::add_linkback(const gint64 id) {
		std::stringstream stream;

		stream << linkbacks->get_label();
		if (linkbacks->get_label().size() > 0) {
			stream << " ";
		}

		stream << "<a href=\"" << id
		       << "\"><span color=\"#0F0C5D\"><span font_size=\"x-small\">&gt;&gt;" << id << "</span></span></a>";

		linkbacks->set_markup(stream.str());

		if ( linkbacks->get_parent() == nullptr ) {
			viewport_grid->insert_next_to(*post_info_grid, Gtk::POS_BOTTOM);
			viewport_grid->attach_next_to(*linkbacks, *post_info_grid, Gtk::POS_BOTTOM, 1, 1);
			linkbacks->show();
			viewport_grid->queue_resize();
		}

		queue_resize();
	}

	void PostView::refresh(const Glib::RefPtr<Post> &in) {
		post = in;
	}

	bool PostView::on_activate_link(const Glib::ustring& link) {
		return signal_activate_link(link);
	}
}

