#ifndef POST_VIEW_HPP
#define POST_VIEW_HPP
#include "thread.hpp"
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/texttagtable.h>
#include <gtkmm/textview.h>
#include <gtkmm/viewport.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/image.h>
#include <giomm/dbusproxy.h>
#include "image_fetcher.hpp"
#include "horizon_image.hpp"

namespace Horizon {

	class PostView : public Gtk::Grid {
	public:
		PostView( const Glib::RefPtr<Post> &in );
		virtual ~PostView();

		void refresh( const Glib::RefPtr<Post> &in );
		sigc::signal<bool, const Glib::ustring&> signal_activate_link;
		void add_linkback(const gint64 id);
		void set_comment_grid();
		
		Glib::ustring get_comment_body() const;
		Glib::RefPtr<Gdk::Pixbuf> get_image() const;
		void set_image_state(const Image::ImageState new_state);

	private:
		PostView() = delete;
		PostView(const PostView&) = delete;
		PostView& operator=(const PostView&) = delete;

		Glib::RefPtr<Post> post;

		Gtk::Grid* post_info_grid;
		Gtk::Grid* image_info_grid;
		Gtk::Grid* content_grid;  // Contains the image and the comment
		Gtk::Grid* viewport_grid; // Contains comments and special widgets for 
		                          // things like code tags.
		Gtk::EventBox *comment_box;
		Gtk::Label* comment;
		Gtk::Label* linkbacks;

		Image* image;

		void set_new_scaled_image(const int width);
		bool on_activate_link(const Glib::ustring&);
		void on_image_state_changed(const Image::ImageState &state);
		bool on_plug_removed(guint64);
		/*
		Gtk::TextView textview;
		Glib::RefPtr<Gtk::TextBuffer> comment;
		Glib::RefPtr<Gtk::TextTagTable> tag_table;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		*/
	};
}

#endif
