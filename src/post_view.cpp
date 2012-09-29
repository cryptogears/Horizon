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
#include <gtkmm/cssprovider.h>
#include "html_parser.hpp"
#include "horizon_image.hpp"

namespace Horizon {

	PostView::PostView(const Glib::RefPtr<Post> &in) :
		Gtk::Grid(),
		comment(),
		comment_grid(),
		vadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		hadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		comment_viewport(hadjust, vadjust),
		post(in)
	{
		set_name("postview");
		set_orientation(Gtk::ORIENTATION_VERTICAL);
		
		auto ngrid = Gtk::manage(new Gtk::Grid());
		ngrid->set_name("posttopgrid");
		ngrid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		ngrid->set_column_spacing(5);

		auto label = Gtk::manage(new Gtk::Label(post->get_subject()));
		label->set_name("subject");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post->get_name()));
		label->set_name("name");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post->get_time_str()));
		label->set_name("time");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label("No. " + post->get_number()));
		label->set_hexpand(false);

		label->set_justify(Gtk::JUSTIFY_LEFT);
		ngrid->add(*label);

		add(*ngrid);
		
		// Image info
		if ( post->has_image() ) {
			ngrid = Gtk::manage(new Gtk::Grid());
			ngrid->set_name("imageinfogrid");
			ngrid->set_column_spacing(5);

			std::string filename = post->get_original_filename() + post->get_image_ext();

			label = Gtk::manage(new Gtk::Label(filename));
			label->set_name("originalfilename");
			label->set_hexpand(false);
			label->set_ellipsize(Pango::ELLIPSIZE_END);
			ngrid->add(*label);

			const gint64 original_id = g_ascii_strtoll(post->get_original_filename().c_str(), NULL, 10);
			const gint64 original_time  = original_id / 1000;
			if (original_time > 1000000000 && original_time < 10000000000) {
				Glib::DateTime dtime = Glib::DateTime::create_now_local(original_time);
				label = Gtk::manage(new Gtk::Label(dtime.format("%B %-e, %Y")));
				label->set_hexpand(false);
				label->set_name("imagenameinfo");
				ngrid->add(*label);
			} 

			std::stringstream imgdims;
			imgdims << " (" << post->get_fsize() / 1000 
			        << " KB, " << post->get_width() 
			        << "x" << post->get_height() << ") ";

			label = Gtk::manage(new Gtk::Label(imgdims.str()));
			label->set_hexpand(false);
			label->set_justify(Gtk::JUSTIFY_LEFT);
			label->set_name("imagenameinfo");
			ngrid->add(*label);
			add(*ngrid);
		}

		auto parser = HtmlParser::getHtmlParser();
		comment.set_markup(parser->html_to_pango(post->get_comment()));

		comment.set_name("comment");
		comment.set_selectable(true);
		comment.set_valign(Gtk::ALIGN_START);
		comment.set_halign(Gtk::ALIGN_START);
		comment.set_justify(Gtk::JUSTIFY_LEFT);
		comment.set_line_wrap(true);
		comment.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);

		comment_grid.set_name("commentgrid");
		comment_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		comment_grid.add(comment);
		comment_viewport.set_name("commentview");
		comment_viewport.add(comment_grid);
		comment_viewport.set_hexpand(true);
		add(comment_viewport);

		if ( post->has_image() ) {
			image = Image::create(post, &comment_grid);
		}
	}

	void PostView::refresh(const Glib::RefPtr<Post> &in) {
		post = in;
	}
}

