#include <giomm/file.h>
#include <gtkmm/application.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/refptr.h>
#include <gtkmm/main.h>
#include <sigc++/connection.h>
#include <giomm/dbusproxy.h>
#include <gtkmm/main.h>
#include <gtkmm/applicationwindow.h>
#include <vector>
#include <map>
#include <gtkmm/grid.h>

#include "manager.hpp"
#include "thread_view.hpp"
#include <glibmm/variant.h>

namespace Horizon {
	class Application : public Gtk::Application {
	public:
		virtual ~Application();
		
		static Glib::RefPtr<Application> create(const Glib::ustring &appid);

		void onUpdates();


	protected:
		virtual void on_activate();
		virtual void on_startup();
		Application(const Glib::ustring &appid);

	private:
		void setup_actions();
		void setup_window();
		void on_thread_closed(const gint64 id);

		std::map<gint64, ThreadView*> thread_map;

		Horizon::Manager manager;
		sigc::connection manager_alarm;
		
		Glib::RefPtr<Gtk::ApplicationWindow> window;
		Gtk::Grid* grid;
		std::vector<Gtk::Grid*> rows;

		void on_open_thread(const Glib::VariantBase& parameter);
		void on_open_thread_dialog(const Glib::VariantBase&);
		void on_add_row(const Glib::VariantBase&);
		void on_remove_row(const Glib::VariantBase&);
		void on_rows_changed(const std::vector<Gtk::Widget*> &widgets);
		void on_fullscreen(const Glib::VariantBase&);
	};

	static const Glib::ustring app_id = "com.talisein.fourchan.native.gtk";
}
