#include <iostream>
#include "application.hpp"
#include "thread.hpp"
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>
#include <giomm/simpleaction.h>
extern "C" {
#include "horizon-resources.h"
}

namespace Horizon {

	void Application::setup_window() {
		if (!window) {
			GtkApplicationWindow *win = GTK_APPLICATION_WINDOW(gtk_application_window_new(gobj()));
			window = Glib::wrap(win, true);
			grid = Gtk::manage(new Gtk::Grid());
		
			window->set_title("Horizon - A Native GTK+ 4chan Viewer");
			grid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
			grid->set_vexpand(true);
			window->set_default_size(500, 800);
			window->add(*grid);

			auto provider = Gtk::CssProvider::get_default();

			GResource* resource = horizon_get_resource();
			g_resources_register(resource);
			auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/style.css");
			auto sc = window->get_style_context();
			auto screen = Gdk::Screen::get_default();
			provider->load_from_file(file);
			sc->add_provider_for_screen(screen, provider, 600);
			gtk_application_add_window(gobj(), GTK_WINDOW(window->gobj()));

			window->show_all();
		} else {
			g_warning("Startup called twice");
		}
	}

	void Application::on_activate() {
		Gtk::Application::on_activate();

	}

	void Application::on_startup() {
		Gtk::Application::on_startup();
		Glib::set_application_name("Horizon");

		setup_window();
		setup_actions();

		auto t2 = Thread::create("http://boards.4chan.org/a/res/71776852");
		auto tv2 = Gtk::manage(new ThreadView(t2));
		grid->add(*tv2);
		thread_map.insert({t2->id, tv2});
		manager.addThread(t2);


		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::checkThreads), 3);
		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));

	}

	void Application::setup_actions() {
		auto open = Gio::SimpleAction::create("app.open_thread",  // name
		                                      Glib::VARIANT_TYPE_STRING
		                                      );
		open->signal_activate().connect( sigc::mem_fun(*this, &Application::on_open_thread) );
		open->set_enabled(true);
		add_action(open);
	}

	void Application::on_open_thread(const Glib::VariantBase& parameter) {
		std::cerr << "Action activated!" << std::endl;
	}
	

	void Application::onUpdates() {
		while (manager.is_updated_thread()) {
			gint64 tid = manager.pop_updated_thread();
			if (G_LIKELY(tid != 0)) {
				auto iter = thread_map.find(tid);
				if (G_LIKELY( iter != thread_map.end() )) {
					iter->second->refresh();
				} else {
					g_error("Thread %d not in application.thread_map", tid);
				}
			} else {
				g_warning("pop_updated_thread returned tid = 0");
			}
		}
	}

	Application::~Application() {
		manager_alarm.disconnect();
	}


	Application::Application(const Glib::ustring &appid) :
		Gtk::Application(appid),
		grid(nullptr)
	{
	}

	Glib::RefPtr<Application> Application::create(const Glib::ustring &appid) {
		return Glib::RefPtr<Application>( new Application(appid) );
	}

}
