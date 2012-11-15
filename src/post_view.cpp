#include "post_view.hpp"

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
#include "html_parser.hpp"
#include "horizon_image.hpp"
#include <giomm/unixoutputstream.h>
#include <glibmm/fileutils.h>
#include <glibmm/spawn.h>

namespace Horizon {

	PostView::PostView(const Glib::RefPtr<Post> &in) :
		Gtk::Grid(),
		post(in),
		post_info_grid(Gtk::manage(new Gtk::Grid())),
		image_info_grid(Gtk::manage(new Gtk::Grid())),
		content_grid(Gtk::manage(new Gtk::Grid())),
		viewport_grid(Gtk::manage(new Gtk::Grid())),
		comment_box(Gtk::manage(new Gtk::EventBox())),
		comment(Gtk::manage(new Gtk::Label())),
		linkbacks(Gtk::manage(new Gtk::Label())),
		image(nullptr)
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
		comment_box->set_visible_window(true);
		add(*comment_box);
		set_hexpand(true);

		if ( post->has_image() ) {
			auto s = sigc::mem_fun(*this, &PostView::on_image_state_changed);
			image = Gtk::manage(new Image(post, s));
			image->set_hexpand(false);
		}
	}

	PostView::~PostView() {
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

	void PostView::set_comment_grid() {
		auto parser = HtmlParser::getHtmlParser();
		auto strings = parser->html_to_pango(post->get_comment(), post->get_thread_id());
		
		comment->set_markup(strings.front());
		if (strings.size() > 1) {
			bool is_code = true;
			auto iter = strings.begin();
			iter++;
			for ( ; iter != strings.end(); iter++) {
				if (is_code) {
					Gtk::Socket* socket = Gtk::manage(new Gtk::Socket());
					socket->set_size_request(800, 300);
					socket->set_hexpand(true);
					viewport_grid->add(*socket);
					guint64 id = socket->get_id();
					socket->signal_plug_removed().connect(sigc::bind(sigc::mem_fun(*this, &PostView::on_plug_removed),id));
					std::string filename;
					std::stringstream prefix;
					prefix << "Horizon_" << post->get_id() << "_";
					int fd = Glib::file_open_tmp(filename, prefix.str());
					auto ostream_ptr = Gio::UnixOutputStream::create(fd, true);
					ostream_ptr->write(*iter);
					ostream_ptr->close();
					std::stringstream cmdline;
					cmdline << "gvim --servername " << id << " --socketid " << id << " " << filename;
					Glib::spawn_command_line_async(cmdline.str());
				} else {
					Gtk::Label* c = Gtk::manage(new Gtk::Label());
					c->set_markup(*iter);
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
		viewport_grid->show_all();
		comment->queue_resize();
	}

	bool PostView::on_plug_removed(guint64 id) {
		std::cerr << "On_plug_removed() " << id << std::endl;
		return true;
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
			queue_resize();
		}
	}

	void PostView::refresh(const Glib::RefPtr<Post> &in) {
		post = in;
	}

	bool PostView::on_activate_link(const Glib::ustring& link) {
		return signal_activate_link(link);
	}
}

