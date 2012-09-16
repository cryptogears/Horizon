#include <giomm/file.h>
#include <gtkmm/application.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/refptr.h>
#include <gtkmm/main.h>
#include <sigc++/connection.h>
#include <giomm/dbusproxy.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <vector>
#include <map>
#include <gtkmm/grid.h>

#include "manager.hpp"
#include "thread_view.hpp"


namespace Horizon {
	class Application : public Gtk::Application {
	public:
		Application(int& argc, char**& argv, const Glib::ustring& application_id);
		~Application();

		static Glib::RefPtr<Application> create(int &argc, char **& argv);

		void onUpdates();
		int run();

	private:
		std::map<gint64, ThreadView*> thread_map;

		Horizon::Manager manager;
		sigc::connection manager_alarm;
		
		Gtk::Window window;
		Gtk::Grid grid;

	};

	static const Glib::ustring app_id = "com.talisein.fourchan.native.gtk";
}
