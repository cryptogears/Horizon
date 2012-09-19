#include <iostream>
#include "application.hpp"
#include "thread.hpp"
#include <gtkmm/cssprovider.h>
#include <giomm/file.h>
extern "C" {
#include "horizon-resources.h"
}

namespace Horizon {
	Application::Application(int& argc, char**& argv, const Glib::ustring& application_id) :
		Gtk::Application(argc, argv, application_id ),
		window(Gtk::WINDOW_TOPLEVEL)
	{
		add_window(window);
		window.set_title("Horizon - A Native GTK+ 4chan Viewer");
		grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		grid.set_vexpand(true);
		window.add(grid);

		auto provider = Gtk::CssProvider::get_default();

		GResource* resource = horizon_get_resource();
				g_resources_register(resource);
		auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/style.css");
		auto sc = window.get_style_context();
		auto screen = Gdk::Screen::get_default();
				provider->load_from_file(file);
				sc->add_provider_for_screen(screen, provider, 600);



		auto t = Thread::create("http://boards.4chan.org/g/res/27707130");
		auto tv = Gtk::manage(new ThreadView(t));
		grid.add(*tv);
		thread_map.insert({t->id, tv});
		manager.addThread(t);


		auto t2 = Thread::create("http://boards.4chan.org/a/res/71802569");
		auto tv2 = Gtk::manage(new ThreadView(t2));
		grid.add(*tv2);
		thread_map.insert({t2->id, tv2});
		manager.addThread(t2);

		auto t3 = Thread::create("http://boards.4chan.org/a/res/71800981");
		auto tv3 = Gtk::manage(new ThreadView(t3));
		grid.add(*tv3);
		thread_map.insert({t3->id, tv3});
		manager.addThread(t3);


		window.set_default_size(500, 800);

		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::checkThreads), 3);
		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));
		window.show_all();
	}
	
	Application::~Application() {
		manager_alarm.disconnect();
	}

	Glib::RefPtr<Horizon::Application> Application::create(int &argc, char **&argv) {

		return Glib::RefPtr<Horizon::Application>( new Horizon::Application(argc, argv, Horizon::app_id) );
	}

	void Application::onUpdates() {
		Glib::Mutex::Lock lock(manager.data_mutex);
		for ( auto iter = manager.updatedThreads.begin();
		      iter != manager.updatedThreads.end(); iter = manager.updatedThreads.begin()) {
			thread_map[*iter]->refresh();
			manager.updatedThreads.erase(iter);
		}
	}

	int Application::run() {
		return Gtk::Application::run(window);
	}
}
