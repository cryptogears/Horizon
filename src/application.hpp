#include <vector>
#include <map>
#include <sigc++/connection.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/refptr.h>
#include <glibmm/variant.h>
#include <giomm/settings.h>
#include <giomm/file.h>
#include <giomm/dbusproxy.h>
#include <gtkmm/application.h>
#include <gtkmm/main.h>
#include <gtkmm/main.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/grid.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/notebook.h>

#include "manager.hpp"
#include "thread_view.hpp"
#include "summary_cellrenderer.hpp"

namespace Horizon {
	class Application : public Gtk::Application {
	public:
		virtual ~Application();
		
		static Glib::RefPtr<Application> create(const Glib::ustring &appid);
		
		void onUpdates();
		void on_catalog_update();

	protected:
		virtual void on_activate();
		virtual void on_startup();
		Application(const Glib::ustring &appid);

	private:
		void setup_actions();
		void setup_window();
		// When the ThreadView is closed, we remove from thread_map
		void on_thread_closed(const gint64 id);
		// When the thread 404s, we remove from threads
		void remove_thread(const gint64 id);

		bool on_treeview_click(GdkEventButton*);

		std::map<gint64, ThreadView*> thread_map;

		Horizon::Manager manager;
		std::shared_ptr<Canceller> canceller;
		sigc::connection manager_alarm;
		sigc::connection summary_alarm;
		
		Glib::RefPtr<Gio::Settings> settings;
		std::vector<Glib::ustring> threads;
		Glib::RefPtr<Gtk::ApplicationWindow> window;
		Gtk::Grid total_grid;
		Gtk::Grid summary_grid;
		Gtk::Notebook notebook;

		Gtk::TreeView summary_view;
		void refresh_catalog_view();
		bool erase_iter_if_match_id(const Gtk::TreeModel::iterator& iter, const gint64 id);
		void on_catalog_board_change();
		void on_catalog_image(const Glib::RefPtr<Gdk::PixbufLoader> &loader,
		                      Glib::RefPtr<ThreadSummary> thread);
		Glib::RefPtr<Gtk::ListStore> model;
		Gtk::ComboBoxText *board_combobox;
		bool board_combobox_add_board(const std::string& board,
		                               const Glib::ustring& active_board);

		void on_open_thread(const Glib::VariantBase& parameter);
		void on_open_thread_dialog(const Glib::VariantBase&);
		void on_add_row(const Glib::VariantBase&);
		void on_remove_row(const Glib::VariantBase&);
		void on_rows_changed(const std::vector<Gtk::Widget*> &widgets);
		void on_fullscreen(const Glib::VariantBase&);
		void on_board_toggle(const Glib::VariantBase&, const std::string &board);
		
		class MyModelColumns : public Gtk::TreeModel::ColumnRecord
		{
		public:
			Gtk::TreeModelColumn<Glib::ustring> teaser;
			Gtk::TreeModelColumn<Glib::ustring> url;
			Gtk::TreeModelColumn<gint64> reply_count;
			Gtk::TreeModelColumn<gint64> image_count;
			Gtk::TreeModelColumn<gint64> unix_date;
			Gtk::TreeModelColumn<gint64> id;
			Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > thumb;
			Gtk::TreeModelColumn<Glib::RefPtr<ThreadSummary> > thread_summary;
			Gtk::TreeModelColumn<float> ppm;

			MyModelColumns() {add(thread_summary); add(teaser); add(reply_count);
				add(image_count); add(unix_date); add(url);
				add(id); add(thumb); add(ppm);}
		} const thread_summary_columns;
		bool on_search_equal(const Glib::RefPtr<Gtk::TreeModel>& model,
		                     int column,
		                     const Glib::ustring& key,
		                     const Gtk::TreeModel::iterator& iter);
	};

	static const Glib::ustring app_id = "com.talisein.fourchan.native.gtk";

	/*
	 * If you add a board below, you need to update menus.xml and the
	 * GSettings XML file.
	 */
	static const std::vector< std::string > CATALOG_BOARDS = {
		"a", "c", "g", "k", "m", "o", "p", "v", "vg", "w", "cm", "3", "adv",
		"an", "cgl", "ck", "co", "diy", "fa", "fit", "int", "jp", "lit", "mlp",
		"mu", "n", "po", "sci", "sp", "tg", "toy", "trv", "tv", "vp", "wsg",
		"x", "q" };
		
}
