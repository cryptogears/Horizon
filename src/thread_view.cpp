#include "thread_view.hpp"
#include "post_view.hpp"
#include <gtkmm/label.h>
#include <functional>
#include <iostream>
#include <memory>
#include <gtkmm/separator.h>
#include <gtkmm/button.h>
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>
#include <gtkmm/stock.h>
#include "thread.hpp"
#include "html_parser.hpp"
#include "horizon_image.hpp"
#include "image_fetcher.hpp"

extern "C" {
#include "horizon-resources.h"
}

namespace Horizon {

	ThreadView::ThreadView(std::shared_ptr<Thread> t,
	                       Glib::RefPtr<Gio::Settings> s) :
		Gtk::Frame       (),
		thread           (t),
		swindow          (Gtk::manage(new Gtk::ScrolledWindow())),
		vadjustment      (swindow->get_vadjustment()),
		full_grid        (Gtk::manage(new Gtk::Grid())),
		grid             (Gtk::manage(new Gtk::Grid())),
		control_grid     (Gtk::manage(new Gtk::Grid())),
		notify_switch    (Gtk::manage(new Gtk::Switch())),
		autoscroll_switch(Gtk::manage(new Gtk::Switch())),
		expand_button    (Gtk::manage(new Gtk::ToggleButton("Expand All"))),
		tab_window       (Gtk::manage(new Gtk::EventBox())),
		tab_label        (Gtk::manage(new Gtk::Label())),
		tab_label_grid   (Gtk::manage(new Gtk::Grid())),
		tab_image        (Gtk::manage(new Gtk::Image())),
		fetching_image   (false),
		settings         (s),
		notifier         (Notifier::getNotifier()),
		canceller        (std::make_shared<Canceller>())
	{
		set_name("frame");
		set_shadow_type(Gtk::SHADOW_IN);
		set_label_align(0.5f, 0.5f);
		swindow->set_name("threadview");
		swindow->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		swindow->set_vexpand(true);
		grid->set_focus_vadjustment(vadjustment);
		grid->set_row_spacing(5);
		grid->set_column_spacing(5);
		grid->set_margin_right(5);
		grid->set_margin_left(5);
		grid->set_margin_top(5);

		full_grid->set_orientation(Gtk::ORIENTATION_VERTICAL);

		grid->set_orientation(Gtk::ORIENTATION_VERTICAL);
		grid->set_name("postview_grid");

		control_grid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		control_grid->set_border_width(2);
		control_grid->set_column_spacing(5);

		expand_button->set_name("expand_all");
		expand_button->signal_toggled().connect( sigc::mem_fun(*this, &ThreadView::on_expand_all) );
		control_grid->add(*expand_button);

		Gtk::Label *notify_label = Gtk::manage(new Gtk::Label("Notify on New "));
		control_grid->add(*notify_label);
		control_grid->add(*notify_switch);
		Gtk::Label *autoscroll_label = Gtk::manage(new Gtk::Label("Auto-Scroll "));
		control_grid->add(*autoscroll_label);
		control_grid->add(*autoscroll_switch);


		Gtk::Button *close = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));
		close->set_name("closebutton");
		close->signal_clicked().connect( sigc::mem_fun(*this, &ThreadView::do_close_thread) );

		
		control_grid->add(*close);
		control_grid->set_name("threadcontrolgrid");
		full_grid->add(*control_grid);

		swindow->add(*grid);
		full_grid->add(*swindow);
		add(*full_grid);
		
		tab_label->set_text( Glib::ustring::compose("%1", thread->number));
		set_label(tab_label->get_text());

		tab_label_grid->set_column_spacing(10);
		tab_label_grid->show();
		tab_label->set_valign(Gtk::ALIGN_CENTER);
		tab_label->set_vexpand(true);
		tab_image->set_valign(Gtk::ALIGN_CENTER);
		
		tab_label->show();
		tab_label_grid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		tab_label_grid->add(*tab_image);
		tab_label_grid->add(*tab_label);
		tab_label_grid->set_valign(Gtk::ALIGN_CENTER);

		tab_window->set_name("tab");
		tab_window->set_visible_window(false);
		tab_window->add(*tab_label_grid);
		tab_window->show();
		
		thread->signal_updated_interval.connect( sigc::mem_fun(*this, &ThreadView::on_updated_interval ) );
		vadjustment->signal_changed().connect( sigc::mem_fun(*this, &ThreadView::on_scrollbar_changed) );
		prev_upper = 0.;
		prev_page_size = 0.;
		prev_value = 0.;

		show_all();
		
		auto slot = sigc::mem_fun(*this, &ThreadView::refresh_tab_text);
		tab_updates = Glib::signal_timeout().connect_seconds(sigc::bind_return(slot, true), 10);
	}

	ThreadView::~ThreadView() {
		canceller->cancel();
		if (tab_updates.connected())
			tab_updates.disconnect();

		if (unshown_view_idle.connected())
			unshown_view_idle.disconnect();

	}

	void ThreadView::on_scrollbar_changed() {
		const double new_upper = vadjustment->get_upper();
		const double page_size = vadjustment->get_page_size();
		const double value = vadjustment->get_value();
		const double min_increment = vadjustment->get_minimum_increment();
		/* Is it magic?
		   Is it  a demon?
		   Is it     divine?
		   Program   zot    random.

		                 Fly!
		                        you fools.
		*/
		if ( prev_upper != new_upper &&
		     autoscroll_switch->get_active() ) {
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

	bool ThreadView::refresh_post(const Glib::RefPtr<Post> &post) {
		bool was_new = false;
		auto parser = HtmlParser::getHtmlParser();

		if ( post_map.count(post->get_id()) > 0 ) {
			// This post is already in the view
			if ( ! post->is_rendered() ) {
				post_map[post->get_id()]->refresh(post);
				post->mark_rendered();
				was_new = true;
			}
		} else {
			// This is a new post
			PostView *pv = Gtk::manage(new PostView(post));
			post_map.insert({post->get_id(), pv});
			pv->signal_activate_link.connect(sigc::mem_fun(*this, &ThreadView::on_activate_link));
			post->mark_rendered();
			grid->add(*pv);
			was_new = true;

			unshown_views.push_back(pv);

			auto links = parser->get_links(post->get_comment());
			for ( auto link : links ) {
				auto iter = post_map.find(link);
				if (iter != post_map.end()) {
					iter->second->add_linkback(post->get_id());
				}
			}

			if (expand_button->get_active())
				pv->set_image_state(Image::EXPAND);
		}

		if (!unshown_view_idle.connected())
			unshown_view_idle = Glib::signal_idle().connect(sigc::mem_fun(*this, &ThreadView::on_unshown_views));
		return was_new;
	}

	bool ThreadView::on_unshown_views() {
		if (unshown_views.size() > 0) {
			PostView* view = unshown_views.front();
			view->show_all();
			view->set_comment_grid(); // Must be done here so gVim spawns ok
			unshown_views.pop_front();
		} 

		if (unshown_views.size() == 0) {
			unshown_view_idle.disconnect();
			return false;
		} else {
			return true;
		}
	}

	bool ThreadView::refresh() {
		bool was_new = false;
		auto functor = std::bind(std::mem_fn(&ThreadView::refresh_post),
		                         this, std::placeholders::_1);
		was_new = thread->for_each_post(functor);
		thread->update_notify(was_new);

		bool should_notify = true;
		if (settings) {
			Glib::ustring key = "notify-idle";
			should_notify = settings->get_boolean(key);
		}

		if ( was_new &&
		     ( (should_notify && thread->should_notify()) ||
		       notify_switch->get_active())) {
			auto iter = post_map.rbegin();
			
			notifier->notify(thread->id,// id,
			                 "New 4chan post",// summary,
			                 iter->second->get_comment_body(), // body,
			                 "4chan-icon",
			                 iter->second->get_image());
		}

		if (!tab_image->get_pixbuf())
			refresh_tab_image();

		refresh_tab_text();

		return thread->is_404;
	}

	void ThreadView::refresh_tab_image() {
		if (!fetching_image) {
			fetching_image = true;
			auto post = thread->get_first_post();
			if (post) {
				auto cb = std::bind(&ThreadView::set_tab_image, this, std::placeholders::_1);
				auto ifetcher = ImageFetcher::get(FOURCHAN);
				ifetcher->download(post, cb, canceller);
			}
		}
	}

	void ThreadView::set_tab_image(const Glib::RefPtr<Gdk::PixbufLoader> &loader) {
		if (loader) {
			auto pixbuf = loader->get_pixbuf();
			int new_width, new_height;
			float scale;
			new_width = 100;
			scale = static_cast<float>(new_width) / static_cast<float>(pixbuf->get_width());
			new_height = static_cast<int>(scale * static_cast<float>(pixbuf->get_height()));
			if (new_height > 100) {
				new_height = 100;
				scale = static_cast<float>(new_height) / static_cast<float>(pixbuf->get_height());
				new_width = static_cast<int>(scale * static_cast<float>(pixbuf->get_width()));
			}

			tab_image->set(pixbuf->scale_simple(new_width, new_height, Gdk::INTERP_HYPER));
			tab_image->show();
		}

		fetching_image = false;
	}


	void ThreadView::refresh_tab_text() {
		gsize posts, images;
		posts = post_map.size();
		images = thread->get_image_count();
		Glib::ustring lastpost, label;

		Glib::DateTime now = Glib::DateTime::create_now_utc();

		auto diff = std::abs(now.difference(thread->last_post)) / 1000000;
		if ( diff < 60 ) {
			lastpost = Glib::ustring::compose("%1 s", diff);
		} else {
			diff = diff / 60; // Now in minutes
			if ( diff < 60 ) {
				lastpost = Glib::ustring::compose("%1 min%2",
				                                  diff,
				                                  diff>1?"s":"");
			} else {
				diff = diff / 60; // Now in hours
				if (diff < 24) {
					lastpost = Glib::ustring::compose("%1 hour%2",
					                                  diff,
					                                  diff>1?"s":"");
				} else {
					diff = diff / 24; // Now in days
					if (diff < 28) {
						lastpost = Glib::ustring::compose("%1 day%2",
						                                  diff,
						                                  diff>1?"s":"");
					} else {
						auto date = thread->last_post.to_local();
						lastpost = date.format("%b %Y");
					}
				}
			}
		}

		if (!thread->is_404) {
			label = Glib::ustring::compose("<tt>R</tt>: <b>%1</b>\n"
			                               "<tt>I</tt>: <b>%2</b>\n"
			                               "\n"
			                               "<tt>LP</tt>: %3",
			                               posts,
			                               images,
			                               lastpost);
		} else {
			label = Glib::ustring::compose("<tt>R</tt>: <b>%1</b>\n"
			                               "<tt>I</tt>: <b>%2</b>\n"
			                               "<tt>LP</tt>: %3\n"
			                               "Thread 404",
			                               posts,
			                               images,
			                               lastpost);
		}

		tab_label->set_markup(label);
	}

	bool ThreadView::on_activate_link(const Glib::ustring &link) {
		gint64 post_num;
		std::stringstream s;
		if ( link.find("http://") != link.npos ) {
			return false;
		}
		s << link;
		s >> post_num;
		if ( post_map.count(post_num) == 1 ) {
			auto widget = post_map[post_num];
			grid->set_focus_child(*widget);
			return true;
		} else {
			std::cerr << "Cross-thread links not yet supported." << std::endl;
			return true;
		}
	}
	

	void ThreadView::on_updated_interval() {
		if (!tab_image->get_pixbuf())
			refresh_tab_image();

		const Glib::RefPtr<Post> post = thread->get_first_post();

		if (post) {
			Glib::ustring subject, name, interval, label_markup;

			if ( post->get_subject().size() > 0 ) {
				subject = Glib::ustring::compose("<b><span color=\"#0F0C5D\">%1</span></b>",
				                                 post->get_subject());
			} else {
				subject = Glib::ustring::compose("%1", thread->number);
			}
			
			name = Glib::ustring::compose("<b><span color=\"#117743\">%1</span></b>",
			                              post->get_name());

			if (thread->is_404) {
				interval = "<span color=\"#F00\">404</span>";
			} else {
				interval = Glib::ustring::compose("%1s",
				                                  thread->get_update_interval()
				                                  / 1000000);
			}

			label_markup = Glib::ustring::compose("%1 - %2 (<small>%3</small>)",
			                               subject,
			                               name,
			                               interval);

			static_cast<Gtk::Label*>(get_label_widget())->set_markup(label_markup);
		}
	}

	void ThreadView::on_expand_all() {
		if (expand_button->get_active()) {
			std::for_each(post_map.begin(),
			              post_map.end(),
			              [](std::pair<gint64, PostView*> pair) {
				              pair.second->set_image_state(Image::EXPAND);
			              });
		} else {
			std::for_each(post_map.begin(),
			              post_map.end(),
			              [](std::pair<gint64, PostView*> pair) {
				              pair.second->set_image_state(Image::THUMBNAIL);
			              });
		}
	}
}


















