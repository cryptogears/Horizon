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
		vadjustment(get_vadjustment())
	{
		set_name("threadview");
		set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		set_vexpand(true);
		set_hexpand(false);

		grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
		add(grid);

		show_all();
	}

	void ThreadView::refresh() {
		bool was_new = false;
		for ( auto iter = thread->posts.begin();
		      iter != thread->posts.end(); 
		      iter++ ) {
			if ( post_map.count(iter->second.get_id()) > 0 ) {
				// This post is already in the view
				if ( ! iter->second.is_rendered() ) {
					post_map[iter->second.get_id()]->refresh(iter->second);
					iter->second.mark_rendered();
					was_new = true;
				}
			} else {
				// This is a new post
				bool should_notify = false;
				if ( Glib::DateTime::create_now_utc(iter->second.get_unix_time()).difference(thread->last_post) == 0) {
					if (thread->get_update_interval() > NOTIFICATION_INTERVAL || true) {
						should_notify = true;
					}
				}

				PostView *pv = Gtk::manage( new PostView(iter->second, should_notify) );
				post_map.insert({iter->second.get_id(), pv});
				iter->second.mark_rendered();
				grid.add(*pv);
				was_new = true;
				pv->show_all();

			}
			
		}

		if (was_new) {
			//vadjustment->set_value(vadjustment->get_upper());
			//vadjustment->signal_changed();
			//show_all();
			//check_resize();
		} 
		thread->update_notify(was_new);
	}
}


















