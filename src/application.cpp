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
#include <gtkmm/menu.h>
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
			total_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
			grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
			summary_grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
			grid.set_vexpand(true);
			window->set_default_size(500, 800);
			Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
			sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
			sw->set_vexpand(true);

			model = Gtk::ListStore::create(thread_summary_columns);
			summary_view.set_model(model);
			SummaryCellRenderer *cr = Gtk::manage(new SummaryCellRenderer());
			Gtk::TreeViewColumn *column = new Gtk::TreeViewColumn("Threads", *cr);
			column->add_attribute(cr->property_threads(), thread_summary_columns.thread_summary);
			column->set_sort_column(thread_summary_columns.ppm);
			column->set_sort_order(Gtk::SORT_DESCENDING);
			column->set_sort_indicator(true);
			column->set_alignment(Gtk::ALIGN_CENTER);

			Gtk::Entry *search_entry = Gtk::manage(new Gtk::Entry());
			summary_view.set_search_column(thread_summary_columns.teaser);
			summary_view.set_search_entry(*search_entry);
			summary_view.set_search_equal_func(sigc::mem_fun(*this, &Application::on_search_equal));

			summary_view.append_column(* Gtk::manage(column) );
			sw->add(summary_view);
			summary_view.signal_button_press_event().connect(sigc::mem_fun(*this, &Application::on_treeview_click), false);
			board_combobox->signal_changed().connect(sigc::mem_fun(*this, &Application::refresh_catalog_view));
			summary_grid.attach(*search_entry, 0, 0, 1, 1);
			summary_grid.attach(*board_combobox, 1, 0, 1, 1);
			summary_grid.attach(*sw, 0, 1, 2, 1);
			total_grid.add(summary_grid);
			total_grid.add(grid);
			window->add(total_grid);

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

		Glib::wrap_register(horizon_thread_summary_get_type(),
		                    &Horizon::ThreadSummary_Class::wrap_new);
		Horizon::ThreadSummary::get_type();

		settings = Gio::Settings::create("com.talisein.fourchan.native.gtk");

		board_combobox = Gtk::manage(new Gtk::ComboBoxText());
		setup_actions();
		setup_window();

		manager.update_catalogs();

		auto threads = settings->get_string_array("threads");
		for (auto thread : threads) {
			std::cerr << "Opening thread " << thread << " automatically." << std::endl;
			activate_action("open_thread", Glib::Variant<Glib::ustring>::create(thread));
		}


		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::update_threads), 3);

		summary_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::update_catalogs), 30);

	}

	void Application::on_startup() {
		Gtk::Application::on_startup();
		Glib::set_application_name("Horizon");
		GResource* resource = horizon_get_resource();
		g_resources_register(resource);

		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));
		manager.signal_catalog_updated.connect(sigc::mem_fun(*this, &Application::on_catalog_update));
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

		for (auto board : CATALOG_BOARDS) {
			std::stringstream action_name, settings_key;
			action_name << "toggle_board_" << board;
			settings_key << "board-" << board;
			Glib::Variant<bool> board_default;
			settings->get_value(settings_key.str(), board_default);

			auto cboard_toggle = g_simple_action_new_stateful(action_name.str().c_str(),
			                                                  NULL,
			                                                  board_default.gobj());
			auto board_toggle = Glib::wrap(cboard_toggle);

			board_toggle->signal_activate().connect( sigc::bind(sigc::mem_fun(*this, &Application::on_board_toggle), board) );
			board_toggle->set_enabled(true);
			add_action(board_toggle);
			on_board_toggle(board_default, board);
		}

		auto builder = gtk_builder_new();
		GError *error = NULL;
		gtk_builder_add_from_resource(builder, 
		                              "/com/talisein/fourchan/native/gtk/menus.xml",
		                              &error);
		if ( error == NULL ) {
			auto cmenu = gtk_builder_get_object(builder, "app-menu");
			auto menu = Glib::wrap(G_MENU_MODEL(cmenu));

			set_app_menu(menu);
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
					auto tv = Gtk::manage(new ThreadView(t, settings));
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
					threads.push_back(url);
					settings->set_string_array("threads", threads);
					thread_map.insert({t->id, tv});
					manager.add_thread(t);
					manager.update_threads();
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
		grid.add(*gridp);
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

	void Application::on_board_toggle(const Glib::VariantBase &v,
	                                  const std::string &board) {
		Glib::Variant<bool> new_setting;
		const std::string settings_key("board-" + board);
		bool toggle_board = false;
		const bool action_from_menu = v.gobj() == nullptr;
		if ( action_from_menu ) {
			// There was no input, so we toggle the old setting
			toggle_board = !settings->get_boolean(settings_key);
		} else {
			auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(v);
			toggle_board = b.get();
		}

		new_setting = Glib::Variant<bool>::create( toggle_board );
		settings->set_value(settings_key, new_setting);

		if (toggle_board) {
			if (manager.add_catalog_board(board)) {
				board_combobox_add_board(board, "");
				if (board_combobox->get_active_row_number() == -1) 
					board_combobox->set_active(0);
				if (action_from_menu)
					manager.update_catalogs();
			}
		} else {
			if (manager.remove_catalog_board(board)) {
				Glib::ustring oldtext = board_combobox->get_active_text();
				board_combobox->set_active(-1); 
				board_combobox->remove_all();
				auto slot = std::bind(std::mem_fn(&Application::board_combobox_add_board),
				                      this, std::placeholders::_1, oldtext);
				bool at_least_one_was_inserted = manager.for_each_catalog_board(slot);
				if (board_combobox->get_active_row_number() == -1 &&
				    at_least_one_was_inserted)
					board_combobox->set_active(0);
			}
		}
	}

	bool Application::board_combobox_add_board(const std::string &board,
	                                           const Glib::ustring &active_board) {
		std::stringstream label;
		label << "/" << board << "/";
		board_combobox->append(label.str());
		if (active_board.size() > 0 &&
		    active_board.find(label.str()) != active_board.npos)
			board_combobox->set_active_text(label.str());
		return true;
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
			remove_thread(id);
		}
	}

	void Application::remove_thread(const gint64 id) {
		gchar* sid = g_strdup_printf("/res/%" G_GINT64_FORMAT, id);
		for ( auto iter = threads.begin(); iter != threads.end(); iter++) {
			if ( g_str_has_suffix(iter->c_str(), sid) ) {
				threads.erase(iter);
				settings->set_string_array("threads", threads);
				break;
			}
		}
		g_free(sid);
	}

	static const std::string get_board_from_fancy(const std::string &board) {
		const size_t left = board.find_first_of("/");
		const size_t right = board.find_last_of("/");
		return board.substr(left + 1, right - 1);		
	}
	
	void Application::on_catalog_update() {
		// fixme
		summary_view.add_events(Gdk::BUTTON_PRESS_MASK);
		if (board_combobox->get_active_row_number() == -1)
			return;
		const std::string active_board = get_board_from_fancy(board_combobox->get_active_text());


		while (manager.is_updated_catalog()) {
			auto board = manager.pop_updated_catalog_board();
			if (board.size() == active_board.size() &&
			    board.find(active_board) != board.npos) {
				refresh_catalog_view();
			} else {
				try {
					for ( auto thread : manager.get_catalog(board) ) {
						// Get those downloads started. This has to be
						// done from the main thread or else the callbacks
						// will be fucked up.
						thread->fetch_thumb();
					}
				} catch (std::range_error e) {
					;
				}
			}
		}
	}

	void Application::refresh_catalog_view() {
		if (board_combobox->get_active_row_number() == -1)
			return;
		const std::string active_board = get_board_from_fancy(board_combobox->get_active_text());
		try {
			auto catalog = manager.get_catalog(active_board);
		
			if ( model ) {
				model->clear();
				for ( auto thread : catalog ) {
					thread->fetch_thumb();
				
					auto iter = model->append();
					iter->set_value(thread_summary_columns.thread_summary,
					                thread);
					iter->set_value(thread_summary_columns.teaser,
					                Glib::ustring(thread->get_teaser()));
					iter->set_value(thread_summary_columns.url,
					                Glib::ustring(thread->get_url()));
					if (thread->get_thumb_pixbuf())
						iter->set_value(thread_summary_columns.thumb,
						                thread->get_thumb_pixbuf());
					iter->set_value(thread_summary_columns.id,
					                thread->get_id());
					iter->set_value(thread_summary_columns.reply_count,
					                thread->get_reply_count());
					iter->set_value(thread_summary_columns.image_count,
					                thread->get_image_count());
					iter->set_value(thread_summary_columns.ppm, 
					                static_cast<float>(thread->get_reply_count()) /
					                static_cast<float>(Glib::DateTime::
					                                   create_now_utc().to_unix() -
					                                   thread->get_unix_date()));
				}
			} else {
				g_error("Summary model not created");
			}
		} catch (std::range_error e) {
			return;
		}
	}

	void Application::onUpdates() {
		while (manager.is_updated_thread()) {
			gint64 tid = manager.pop_updated_thread();
			if (G_LIKELY(tid != 0)) {
				auto iter = thread_map.find(tid);
				bool is_404 = false;
				if (G_LIKELY( iter != thread_map.end() )) {
					is_404 = iter->second->refresh();
				} else {
					g_warning("Thread %d not in application.thread_map", tid);
				}

				if (is_404) {
					remove_thread(tid);
				}
			} else {
				g_warning("pop_updated_thread returned tid = 0");
			}
		}
	}

	bool Application::on_treeview_click(GdkEventButton* eb) {
		if (eb->type == GDK_2BUTTON_PRESS) {
			if ( summary_view.get_bin_window() == Glib::wrap(eb->window, true) ) {
				Gtk::TreeModel::Path path;
				Gtk::TreeViewColumn *column;
				int cell_x, cell_y;

				summary_view.get_path_at_pos(static_cast<int>(eb->x),
				                             static_cast<int>(eb->y),
				                             path, column, cell_x, cell_y);


				auto iter = model->get_iter(path);
				auto val = iter->get_value(thread_summary_columns.url);
				auto variant = Glib::Variant<Glib::ustring>::create(val);

				activate_action("open_thread", variant);
				std::cerr << "Opening thread " << val << std::endl;
			} 
		}

		return true;
	}

	bool Application::on_search_equal(const Glib::RefPtr<Gtk::TreeModel>& model,
	                                  int column,
	                                  const Glib::ustring& key,
	                                  const Gtk::TreeModel::iterator& iter) {
		auto val = iter->get_value(thread_summary_columns.teaser);
		if ( val.find(key) == val.npos ) {
			return true;
		} else {
			return false;
		}
	}

	Application::~Application() {
		manager_alarm.disconnect();

		for ( auto iter = rows.begin(); iter != rows.end(); iter++ ) {
			delete *iter;
		}

	}


	Application::Application(const Glib::ustring &appid) :
		Gtk::Application(appid)
	{
		grid.show();
		grid.set_row_homogeneous(true);
		auto gridp = new Gtk::Grid();
		grid.add(*gridp);
		gridp->set_vexpand(true);
		gridp->set_column_homogeneous(true);
		rows.push_back(gridp);

		Notifier::init();
	}

	Glib::RefPtr<Application> Application::create(const Glib::ustring &appid) {
		return Glib::RefPtr<Application>( new Application(appid) );
	}

}
