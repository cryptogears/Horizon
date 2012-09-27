#ifndef HORIZON_THREADVIEW_HPP
#define HORIZON_THREADVIEW_HPP

#include <gtkmm/grid.h>
#include <gtkmm/viewport.h>
#include <memory>
#include <map>
#include "thread.hpp"
#include "post_view.hpp"
#include <gtkmm/layout.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/frame.h>

namespace Horizon {

	class ThreadView : public Gtk::Frame {
	public:
		ThreadView(std::shared_ptr<Thread> t);
		void refresh();
		
		sigc::signal<void, gint64> signal_closed;

	private:
		std::shared_ptr<Thread> thread;
		Gtk::ScrolledWindow swindow;
		Glib::RefPtr<Gtk::Adjustment> hadjustment;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		Gtk::Grid full_grid; // holds everything
		Gtk::Grid grid; // holds actual postviews
		Gtk::Grid control_grid;

		std::map<gint64, PostView*> post_map;

		double prev_value;
		double prev_upper;
		double prev_page_size;

		bool refresh_post(const Glib::RefPtr<Post> &post);

		void on_updated_interval();
		void do_close_thread();
		void on_scrollbar_changed();
	};

}


#endif
