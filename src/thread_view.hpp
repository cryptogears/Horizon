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
#include "thread.hpp"
#include "post_view.hpp"
#include "notifier.hpp"

namespace Horizon {

	class ThreadView : public Gtk::Frame {
	public:
		ThreadView(std::shared_ptr<Thread> t, Glib::RefPtr<Gio::Settings>);
		// Returns true is the thread is now 404
		bool refresh();
		
		sigc::signal<void, gint64> signal_closed;

	private:
		std::shared_ptr<Thread> thread;
		Gtk::ScrolledWindow swindow;
		Glib::RefPtr<Gtk::Adjustment> hadjustment;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		Gtk::Grid full_grid; // holds everything
		Gtk::Grid grid; // holds actual postviews
		Gtk::Grid control_grid;
		Gtk::Switch notify_switch;
		Gtk::Switch autoscroll_switch;

		std::map<gint64, PostView*> post_map;
		Glib::RefPtr<Gio::Settings> settings;
		std::shared_ptr<Notifier> notifier;

		double prev_value;
		double prev_upper;
		double prev_page_size;

		bool refresh_post(const Glib::RefPtr<Post> &post);

		bool on_activate_link(const Glib::ustring &link);
		void on_updated_interval();
		void do_close_thread();
		void on_scrollbar_changed();
	};

}


#endif
