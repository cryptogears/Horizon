#include "thread_view.hpp"
#include "post_view.hpp"
#include <gtkmm/label.h>
#include <iostream>
#include <gtkmm/separator.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>
#include <gtkmm/stock.h>

extern "C" {
#include "horizon-resources.h"
}

namespace Horizon {

	ThreadView::ThreadView(std::shared_ptr<Thread> t) :
		thread(t),
		Gtk::Frame(),
		swindow(),
		vadjustment(swindow.get_vadjustment())
	{
		set_name("frame");
		set_shadow_type(Gtk::SHADOW_IN);
		set_label_align(0.5f, 0.5f);
		swindow.set_name("threadview");
		swindow.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		swindow.set_vexpand(true);

		full_grid.set_orientation(Gtk::ORIENTATION_VERTICAL);

		grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
		control_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		control_grid.set_border_width(2);

		Gtk::Button *close = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));
		close->signal_clicked().connect( sigc::mem_fun(*this, &ThreadView::do_close_thread) );

		control_grid.add(*close);
		control_grid.set_name("threadcontrolgrid");
		full_grid.add(control_grid);

		swindow.add(grid);
		full_grid.add(swindow);
		add(full_grid);
		set_label(thread->number);

		thread->signal_updated_interval.connect( sigc::mem_fun(*this, &ThreadView::on_updated_interval ) );
		vadjustment->signal_changed().connect( sigc::mem_fun(*this, &ThreadView::on_scrollbar_changed) );
		prev_upper = 0.;
		prev_page_size = 0.;
		prev_value = 0.;
	}

	void ThreadView::on_scrollbar_changed() {
		const double new_upper = vadjustment->get_upper();
		const double page_size = vadjustment->get_page_size();
		const double value = vadjustment->get_value();
		const double new_value = new_upper - page_size;
		const double min_increment = vadjustment->get_minimum_increment();
		/* Is it magic?
		   Is it  a demon?
		   Is it     divine?
		   Program   zot    random.

		                 Fly!
		                        you fools.
		*/
		if ( (prev_upper != new_upper)
		     &&
		     ((prev_page_size + min_increment - (min_increment / 100))
		      >= ( prev_upper - prev_value ))) {
			if ( new_value > value )
				vadjustment->set_value(new_upper - min_increment);
		}

		prev_value = value;
		prev_upper = new_upper;
		prev_page_size = page_size;
	}

	void ThreadView::do_close_thread() {
		hide();

		signal_closed(thread->id);
	}

	void ThreadView::refresh() {
		bool was_new = false;
		show();
		full_grid.show();
		swindow.show();
		grid.show();
		control_grid.show_all();
		Glib::DateTime prev_posttime = Glib::DateTime::create_now_utc(0);
	
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
				prev_posttime = Glib::DateTime::create_now_utc(iter->second.get_unix_time());
			} else {
				// This is a new post
				bool should_notify = false;
				Glib::DateTime this_posttime = Glib::DateTime::create_now_utc(iter->second.get_unix_time());

				if (this_posttime.difference(thread->last_post) == 0) {
					if (this_posttime.difference(prev_posttime) > NOTIFICATION_INTERVAL) {
						should_notify = true;
					} 
				}
				prev_posttime = this_posttime;

				PostView *pv = Gtk::manage( new PostView(iter->second, should_notify) );
				post_map.insert({iter->second.get_id(), pv});
				iter->second.mark_rendered();
				grid.add(*pv);
				was_new = true;
				pv->show_all();
			}
			
			thread->update_notify(was_new);
			
		} 
	}
	

	void ThreadView::on_updated_interval() {
		auto fpost = thread->posts.begin();
		std::stringstream label;
		if ( fpost->second.get_subject().size() > 0 ) {
			label << "<b><span color=\"#0F0C5D\">" << fpost->second.get_subject()
			      << "</span></b>";
		} else {
			label << thread->number;
		}

		label << " - <b><span color=\"#117743\">"  << fpost->second.get_name()
		      << "</span></b> (<small>" 
		      << thread->get_update_interval() / 1000000 
		      << "s</small>)";

		static_cast<Gtk::Label*>(get_label_widget())->set_markup(label.str());

	}
}


















