#ifndef HORIZON_THREADVIEW_HPP
#define HORIZON_THREADVIEW_HPP

#include <memory>
#include <map>
#include <giomm/settings.h>
#include <gtkmm/grid.h>
#include <gtkmm/viewport.h>
#include <gtkmm/layout.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/frame.h>
#include <gtkmm/switch.h>
#include <gtkmm/togglebutton.h>
#include "thread.hpp"
#include "post_view.hpp"
#include "notifier.hpp"
#include "canceller.hpp"

namespace Horizon {

	class ThreadView : public Gtk::Frame {
	public:
		ThreadView(std::shared_ptr<Thread> t, Glib::RefPtr<Gio::Settings>);
		virtual ~ThreadView();
		// Returns true is the thread is now 404
		bool refresh();
		
		sigc::signal<void, gint64> signal_closed;

		Gtk::Widget* get_tab_label() {return tab_window;};

	private:
		std::shared_ptr<Thread>       thread;
		Gtk::ScrolledWindow          *swindow;
		Glib::RefPtr<Gtk::Adjustment> hadjustment;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		Gtk::Grid                    *full_grid; // holds everything
		Gtk::Grid                    *grid; // holds actual postviews
		Gtk::Grid                    *control_grid;
		Gtk::Switch                  *notify_switch;
		Gtk::Switch                  *autoscroll_switch;
		Gtk::ToggleButton            *expand_button;

		Gtk::EventBox                *tab_window;
		Gtk::Label                   *tab_label;
		Gtk::Grid                    *tab_label_grid;
		Gtk::Image                   *tab_image;
		bool                          fetching_image;
		sigc::connection              tab_updates;

		std::deque<PostView*>         unshown_views;
		bool on_unshown_views();
		sigc::connection              unshown_view_idle;

		std::map<gint64, PostView*>   post_map;
		Glib::RefPtr<Gio::Settings>   settings;
		std::shared_ptr<Notifier>     notifier;

		std::shared_ptr<Canceller>    canceller;

		double                        prev_value;
		double                        prev_upper;
		double                        prev_page_size;

		bool refresh_post(const Glib::RefPtr<Post> &post);
		void refresh_tab_image();
		void set_tab_image(const Glib::RefPtr<Gdk::PixbufLoader> &loader);
		void refresh_tab_text();

		bool on_activate_link(const Glib::ustring &link);
		void on_updated_interval();
		void do_close_thread();
		void on_scrollbar_changed();
		void on_expand_all();
	};

}


#endif
