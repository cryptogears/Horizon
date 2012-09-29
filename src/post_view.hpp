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
		~PostView() = default;

		void refresh( const Glib::RefPtr<Post> &in );

	private:
		PostView() = delete;
		PostView(const PostView&) = delete;
		PostView& operator=(const PostView&) = delete;

		Glib::RefPtr<Post> post;
		Glib::RefPtr<Gtk::Adjustment> hadjust, vadjust;
		Gtk::Grid comment_grid;
		Gtk::Label comment;
		Gtk::Viewport comment_viewport;

		std::shared_ptr<Image> image;

		void set_new_scaled_image(const int width);

		/*
		Gtk::TextView textview;
		Glib::RefPtr<Gtk::TextBuffer> comment;
		Glib::RefPtr<Gtk::TextTagTable> tag_table;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		*/
	};
}

#endif
