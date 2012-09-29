#include <iostream>
#include "application.hpp"
#include "thread.hpp"
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>
#include <giomm/simpleaction.h>
#include <gtkmm/builder.h>
#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <gtkmm/stock.h>
#include "notifier.hpp"

extern "C" {
#include "horizon-resources.h"
}

namespace Horizon {

	void Application::setup_window() {
		if (!window) {
			GtkApplicationWindow *win = GTK_APPLICATION_WINDOW(gtk_application_window_new(gobj()));
			window = Glib::wrap(win, true);
		
			window->set_title("Horizon - A Native GTK+ 4chan Viewer");
			window->set_name("horizonwindow");
			grid->set_orientation(Gtk::ORIENTATION_VERTICAL);
			grid->set_vexpand(true);
			window->set_default_size(500, 800);
			window->add(*grid);

			auto provider = Gtk::CssProvider::get_default();

			auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/style.css");
			auto sc = window->get_style_context();
			auto screen = Gdk::Screen::get_default();
			provider->load_from_file(file);
			sc->add_provider_for_screen(screen, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_application_add_window(gobj(), GTK_WINDOW(window->gobj()));

			window->show_all();
		} else {
			g_warning("Startup called twice");
		}
	}

	void Application::on_activate() {
		Gtk::Application::on_activate();

		setup_window();


	}

	void Application::on_startup() {
		Gtk::Application::on_startup();
		Glib::set_application_name("Horizon");

		GResource* resource = horizon_get_resource();
		g_resources_register(resource);

		setup_actions();

		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::checkThreads), 3);
		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));

	}

	void Application::setup_actions() {
		auto open = Gio::SimpleAction::create("open_thread",  // name
		                                      Glib::VARIANT_TYPE_STRING
		                                      );
		open->signal_activate().connect( sigc::mem_fun(*this, &Application::on_open_thread) );
		open->set_enabled(true);
		add_action(open);

		auto opendialog = Gio::SimpleAction::create("open_thread_dialog");
		opendialog->signal_activate().connect( sigc::mem_fun(*this, &Application::on_open_thread_dialog) );
		opendialog->set_enabled(true);
		add_action(opendialog);

		auto addrow = Gio::SimpleAction::create("add_row");
		addrow->signal_activate().connect( sigc::mem_fun(*this, &Application::on_add_row) );
		addrow->set_enabled(true);
		add_action(addrow);

		auto removerow = Gio::SimpleAction::create("remove_row");
		removerow->signal_activate().connect( sigc::mem_fun(*this, &Application::on_remove_row) );
		removerow->set_enabled(true);
		add_action(removerow);

		auto fullscreen = Gio::SimpleAction::create("fullscreen");
		fullscreen->signal_activate().connect( sigc::mem_fun(*this, &Application::on_fullscreen) );
		fullscreen->set_enabled(true);
		add_action(fullscreen);


		auto builder = gtk_builder_new();
		GError *error = NULL;
		gtk_builder_add_from_resource(builder, 
		                              "/com/talisein/fourchan/native/gtk/menus.xml",
		                              &error);
		if ( error == NULL ) {
			auto menu = gtk_builder_get_object(builder, "app-menu");
			set_app_menu(Glib::wrap(G_MENU_MODEL(menu)));
		} else {
			g_error ( "Error loading menu resource: %s", error->message );
		}

		g_object_unref(builder);
	}

	void Application::on_open_thread_dialog(const Glib::VariantBase&) {
		Gtk::Dialog* dialog = new Gtk::Dialog("Open New Thread");
		auto box = dialog->get_vbox();
		auto label = Gtk::manage(new Gtk::Label("Enter 4chan thread URL:"));
		auto entry = Gtk::manage(new Gtk::Entry());
		auto button = dialog->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
		dialog->set_default_response(Gtk::RESPONSE_OK);
		entry->set_activates_default(true);
		box->add(*label);
		box->add(*entry);
		box->show_all();
		auto resp = dialog->run();
		if ( resp == Gtk::RESPONSE_OK ) {
			auto parameter = Glib::Variant<Glib::ustring>::create(entry->get_text());
			activate_action("open_thread", parameter);
		}
		dialog->hide();
		delete dialog;
	}

	void Application::on_open_thread(const Glib::VariantBase& parameter) {
		Glib::Variant<Glib::ustring> str_variant = Glib::VariantBase::cast_dynamic< Glib::Variant<Glib::ustring> >(parameter);
		Glib::ustring url = str_variant.get();

		if ( url.find("4chan.org") != url.npos ) {
			if ( auto pos = url.find("https") != url.npos ) {
				url.erase(pos + 3, 1);
			}

			if ( url.find("http://") != url.npos ) {
				auto t = Thread::create(url);
				if ( thread_map.count( t->id ) == 0 ) {
					auto tv = Gtk::manage(new ThreadView(t));
					tv->signal_closed.connect( sigc::mem_fun(*this, &Application::on_thread_closed) );
					int least_elems = G_MAXINT;
					Gtk::Grid *insert_grid = nullptr;
					for ( auto iter = rows.begin(); iter != rows.end(); iter++ ) {
						Gtk::Grid *gp = *iter;
						size_t count = gp->get_children().size();
						if ( count < least_elems ) {
							least_elems = count;
							insert_grid = gp;
						}
					}
					if ( insert_grid ) {
						insert_grid->add(*tv);
						insert_grid->show();
					} else {
						g_error ( "Didn't find a grid to insert a threadview." );
					}
					thread_map.insert({t->id, tv});
					manager.addThread(t);
					manager.checkThreads();
				}
			}
		}
	}

	void Application::on_add_row(const Glib::VariantBase&) {
		std::vector<Gtk::Widget*> widgets;
		for ( auto grid_iter = rows.begin(); grid_iter != rows.end(); grid_iter++) {
			std::vector<Gtk::Widget*> row_widgets = (*grid_iter)->get_children();

			for ( auto iter = row_widgets.rbegin(); iter != row_widgets.rend(); iter++) {
				widgets.push_back(*iter);
				(*grid_iter)->remove(**iter);
			}
		}

		auto gridp = new Gtk::Grid();
		gridp->set_vexpand(true);
		grid->add(*gridp);
		gridp->set_column_homogeneous(true);
		rows.push_back(gridp);
		
		on_rows_changed(widgets);
	}

	void Application::on_rows_changed(const std::vector<Gtk::Widget*> &widgets) {
		int i = 0;
		int row = 0;
		while ( i < widgets.size() ) {
			rows[row]->add(*(widgets[i]));
			rows[row]->show();
			i++;
			row++;
			if ( row >= rows.size() )
				row = 0;
		}
	}
	
	void Application::on_remove_row(const Glib::VariantBase&) {
		if ( rows.size() > 1 ) {
			std::vector<Gtk::Widget*> widgets;
			for ( auto grid_iter = rows.begin(); grid_iter != rows.end(); grid_iter++) {
				std::vector<Gtk::Widget*> row_widgets = (*grid_iter)->get_children();
				
				for ( auto iter = row_widgets.begin(); iter != row_widgets.end(); iter++) {
					widgets.push_back(*iter);
					(*grid_iter)->remove(**iter);
				}
			}

			auto grid = rows.back();
			rows.pop_back();
			delete grid;

			on_rows_changed(widgets);
		}
	}

	void Application::on_fullscreen(const Glib::VariantBase&) {

	}

	void Application::on_thread_closed(const gint64 id) {
		auto iter = thread_map.find(id);
		if ( iter != thread_map.end() ) {
			manager.remove_thread(id);

			ThreadView *tv = iter->second;
			auto parent = tv->get_parent();
			parent->remove(*tv);
			if ( parent->get_children().size() == 0)
				parent->hide();
			thread_map.erase(iter);
		}
		
	}

	void Application::onUpdates() {
		while (manager.is_updated_thread()) {
			gint64 tid = manager.pop_updated_thread();
			if (G_LIKELY(tid != 0)) {
				auto iter = thread_map.find(tid);
				if (G_LIKELY( iter != thread_map.end() )) {
					iter->second->refresh();
				} else {
					g_warning("Thread %d not in application.thread_map", tid);
				}
			} else {
				g_warning("pop_updated_thread returned tid = 0");
			}
		}
	}

	Application::~Application() {
		manager_alarm.disconnect();

		for ( auto iter = rows.begin(); iter != rows.end(); iter++ ) {
			delete *iter;
		}

		delete grid;
	}


	Application::Application(const Glib::ustring &appid) :
		Gtk::Application(appid),
		grid(nullptr)
	{
		grid = new Gtk::Grid();
		grid->show();
		grid->set_row_homogeneous(true);
		auto gridp = new Gtk::Grid();
		grid->add(*gridp);
		gridp->set_vexpand(true);
		gridp->set_column_homogeneous(true);
		rows.push_back(gridp);

		Notifier::init();
	}

	Glib::RefPtr<Application> Application::create(const Glib::ustring &appid) {
		return Glib::RefPtr<Application>( new Application(appid) );
	}

}
