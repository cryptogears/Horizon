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
		window.add(grid);

		auto provider = Gtk::CssProvider::get_default();

		std::cout << "Old sheet: " << provider->to_string() << std::endl;

		GResource* resource = horizon_get_resource();
		g_resources_register(resource);
		auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/style.css");
		if ( !file ) {
			g_error("Unable to load CSS Style Sheet");
		}
		if (!provider->load_from_file(file)) {
			g_error("Unable to load the CSS sheet!");
		}

		std::cout << "New sheet: " << provider->to_string() << std::endl;

		auto sc = window.get_style_context();
		auto screen = Gdk::Screen::get_default();
		sc->add_provider_for_screen(screen, provider, 600);


		auto t = Thread::create("http://boards.4chan.org/g/res/27637097");
		auto tv = new ThreadView(t);
		grid.add(*tv);
		thread_map.insert({t->id, tv});
		manager.addThread(t);

		
		/*
		auto t2 = Thread::create("https://boards.4chan.org/a/res/71612123");
		auto tv2 = new ThreadView(t2);
		grid.add(*tv2);
		thread_map.insert({t2->id, tv2});
		manager.addThread(t2);
		*/
		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::checkThreads), 3);
		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));
		window.show_all();
	}
	
	Application::~Application() {
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
		/*
		for ( auto iter = threads.begin(); iter != threads.end(); iter++) {
			std::cout << "Thread #" << (*iter)->id << " ("
			          << (*iter)->posts.size() << ")" << std::endl;
			for ( auto piter = (*iter)->posts.begin(); piter != (*iter)->posts.end(); piter++) {
				if (! piter->second.is_rendered() ) {
					std::cout << "\tComment #" << piter->second.getId()
					          << ": " << piter->second.getComment() 
					          << std::endl;
					piter->second.mark_rendered();
				}
			}
		}
		*/
	}

	int Application::run() {
		return Gtk::Application::run(window);
	}
}
