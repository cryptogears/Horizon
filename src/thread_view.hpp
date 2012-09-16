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

namespace Horizon {

	class ThreadView : public Gtk::ScrolledWindow {
	public:
		ThreadView(std::shared_ptr<Thread> t);
		void refresh();

	private:
		std::shared_ptr<Thread> thread;
		Glib::RefPtr<Gtk::Adjustment> hadjustment;
		Glib::RefPtr<Gtk::Adjustment> vadjustment;
		Gtk::Grid grid;
		std::map<gint64, PostView*> post_map;

	};

}


#endif
