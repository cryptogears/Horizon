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

namespace Horizon {

	class PostView : public Gtk::Grid {
	public:
		PostView( const Glib::RefPtr<Post> &in );
		~PostView();

		void refresh( const Glib::RefPtr<Post> &in );
		const bool should_notify() const;
		const bool should_notify(const bool notify);

	private:
		PostView() = delete;
		PostView(const PostView&) = delete;
		PostView& operator=(const PostView&) = delete;

		static Glib::RefPtr<Gio::DBus::Proxy> get_dbus_proxy();
		void do_notify(const Glib::RefPtr<Gdk::Pixbuf>& = Glib::RefPtr<Gdk::Pixbuf>());

		void on_thumb_ready(std::string hash);
		void on_image_ready(std::string hash);
		sigc::connection thumb_connection;
		sigc::connection image_connection;

		bool notify;


		Glib::RefPtr<Post> post;
		Glib::RefPtr<Gtk::Adjustment> hadjust, vadjust;
		Gtk::Grid comment_grid;
		Gtk::Label comment;
		Gtk::Viewport comment_viewport;

		enum { NONE, THUMBNAIL, EXPAND, FULL } image_state;
		Gtk::Image image;
		Gtk::EventBox image_box;
		Gtk::ScrolledWindow image_window;
		Glib::RefPtr<Gdk::Pixbuf> thumbnail_image;
		Glib::RefPtr<Gdk::Pixbuf> unscaled_image;
		Glib::RefPtr<Gdk::Pixbuf> scaled_image;
		double scaled_width, scaled_height;
		Glib::RefPtr<Gdk::PixbufAnimation> unscaled_animation;
		Glib::RefPtr<Gdk::PixbufAnimationIter> animation_iter;
		bool on_image_draw(const Cairo::RefPtr< ::Cairo::Context>& ctx);
		bool on_image_click(GdkEventButton* btn);
		void set_new_scaled_image(const int width);
		void on_postview_show();
		bool on_my_mapped(GdkEventAny* const&);
		int derp = 0;

		std::shared_ptr<ImageFetcher> ifetcher;
		/*
		Gtk::TextView textview;
		Glib::RefPtr<Gtk::TextBuffer> comment;
		Glib::RefPtr<Gtk::TextTagTable> tag_table;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		*/
	};
}

#endif
