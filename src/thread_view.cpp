#include "thread_view.hpp"
#include "post_view.hpp"
#include <gtkmm/label.h>
#include <iostream>
#include <gtkmm/separator.h>
extern "C" {
#include "horizon-resources.h"
}
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>

namespace Horizon {

	ThreadView::ThreadView(std::shared_ptr<Thread> t) :
		thread(t),
		Gtk::ScrolledWindow(),
		grid()
	{
		grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
		set_name("threadview");
		add(grid);

		vadjustment = get_vadjustment(); 

		Gtk::Label* derp = Gtk::manage( new Gtk::Label(t->number) );
		grid.add(*derp);

		set_size_request(500,1000);

		set_vexpand(true);
		set_hexpand(true);
		grid.set_vexpand(true);
		grid.set_hexpand(true);
		
		set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	}

	void ThreadView::refresh() {
		bool was_new = false;
		for ( auto iter = thread->posts.begin();
		      iter != thread->posts.end(); iter++ ) {
			if ( post_map.count(iter->second.getId()) > 0 ) {
				if ( ! iter->second.is_rendered() ) {
					post_map[iter->second.getId()]->refresh(iter->second);
					iter->second.mark_rendered();
					was_new = true;
				}
			} else {
				PostView *pv = Gtk::manage( new PostView(iter->second) );
				post_map.insert({iter->second.getId(), pv});
				grid.add(*pv);
				pv->show_all();
				auto sep = Gtk::manage( new Gtk::HSeparator() );
				sep->set_margin_bottom(1);
				
				grid.add(*sep);
				was_new = true;
			}
			
		}
		if (was_new) {

			vadjustment->set_value(vadjustment->get_upper());
			vadjustment->signal_changed();
		}
	}
	

}


















