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
			GtkApplicationWindow *win = GTK_APPLICATION_WINDOW(gtk_application_window_new(gapplication->gobj()));
			window = Glib::wrap(win, true);
			window->set_title("Horizon - A Native GTK+ 4chan Viewer");
			window->set_name("horizonwindow");
			total_grid = Gtk::manage(new Gtk::Grid());
			summary_grid = Gtk::manage(new Gtk::Grid());
			total_grid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
			summary_grid->set_orientation(Gtk::ORIENTATION_VERTICAL);
			window->set_default_size(500, 800);
			Gtk::ScrolledWindow* sw = Gtk::manage(new Gtk::ScrolledWindow());
			sw->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
			sw->set_vexpand(true);

			model = Gtk::ListStore::create(thread_summary_columns);
			summary_view = Gtk::manage(new Gtk::TreeView());
			summary_view->set_model(model);
			SummaryCellRenderer *cr = Gtk::manage(new SummaryCellRenderer());
			Gtk::TreeViewColumn *column = Gtk::manage(new Gtk::TreeViewColumn("Threads", *cr));
			column->add_attribute(cr->property_threads(), thread_summary_columns.thread_summary);
			column->set_sort_column(thread_summary_columns.ppm);
			column->set_sort_order(Gtk::SORT_DESCENDING);
			column->set_sort_indicator(true);
			column->set_alignment(Gtk::ALIGN_CENTER);

			Gtk::Entry *search_entry = Gtk::manage(new Gtk::Entry());
			summary_view->set_search_column(thread_summary_columns.teaser);
			summary_view->set_search_entry(*search_entry);
			summary_view->set_search_equal_func(sigc::mem_fun(*this, &Application::on_search_equal));

			summary_view->append_column(*column);
			sw->add(*summary_view);
			summary_view->signal_button_press_event().connect(sigc::mem_fun(*this, &Application::on_treeview_click), false);
			board_combobox->signal_changed().connect(sigc::mem_fun(*this, &Application::on_catalog_board_change));
			summary_grid->attach(*search_entry, 0, 0, 1, 1);
			summary_grid->attach(*board_combobox, 1, 0, 1, 1);
			summary_grid->attach(*sw, 0, 1, 2, 1);
			total_grid->add(*summary_grid);

			notebook = Gtk::manage(new Gtk::Notebook());
			notebook->set_scrollable(true);
			notebook->popup_enable();

			total_grid->add(*notebook);
			window->add(*total_grid);

			auto provider = Gtk::CssProvider::get_default();

			auto file = Gio::File::create_for_uri("resource:///com/talisein/fourchan/native/gtk/style.css");
			auto sc = window->get_style_context();
			auto screen = Gdk::Screen::get_default();
			provider->load_from_file(file);
			sc->add_provider_for_screen(screen, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			gtk_application_add_window(gapplication->gobj(), GTK_WINDOW(window->gobj()));

			window->show_all();
		} else {
			g_warning("Startup called twice");
		}
	}

	/* Called every time the executable is run */
	void Application::on_activate() {
	}


	void Application::on_startup() {
		Glib::set_application_name("Horizon");

		Glib::wrap_register(horizon_thread_summary_get_type(),
		                    &Horizon::ThreadSummary_Class::wrap_new);
		Horizon::wrap_init();

		GResource* resource = horizon_get_resource();
		g_resources_register(resource);

		manager.signal_thread_updated.connect(sigc::mem_fun(*this, &Application::onUpdates));
		manager.signal_catalog_updated.connect(sigc::mem_fun(*this, &Application::on_catalog_update));

		auto schemas = Gio::Settings::list_schemas();
		auto iter = std::find(schemas.begin(),
		                      schemas.end(),
		                      "com.talisein.fourchan.native.gtk");
		if (iter != schemas.end()) {
			GSettings* csettings = g_settings_new("com.talisein.fourchan.native.gtk");

			if (csettings)
				settings = Glib::wrap(csettings);
		} else {
			std::cerr << "Error: GSettings schema not found. No actions in this session will be remembered. Did you 'make install'?" << std::endl;
		}

		board_combobox = Gtk::manage(new Gtk::ComboBoxText());
		setup_actions();
		setup_window();

		manager.update_catalogs();
		if ( settings ) {
			std::vector <Glib::ustring> threads = settings->get_string_array("threads");
			auto iter_end = std::unique(threads.begin(), threads.end());
			auto app = gapplication;
			std::for_each(threads.begin(),
			              iter_end,
			              [&app](const Glib::ustring &thread) {
				              auto vthread = Glib::Variant<Glib::ustring>::create(thread);
				              app->activate_action("open_thread", vthread);
			              });
		}

		manager_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::update_threads), 3);
		summary_alarm = Glib::signal_timeout().connect_seconds(sigc::mem_fun(&manager, &Manager::update_catalogs), 60);
	}

	void Application::setup_actions() {
		auto open = Gio::SimpleAction::create("open_thread",  // name
		                                      Glib::VARIANT_TYPE_STRING
		                                      );
		open->signal_activate().connect( sigc::mem_fun(*this, &Application::on_open_thread) );
		open->set_enabled(true);
		gapplication->add_action(open);

		auto opendialog = Gio::SimpleAction::create("open_thread_dialog");
		opendialog->signal_activate().connect( sigc::mem_fun(*this, &Application::on_open_thread_dialog) );
		opendialog->set_enabled(true);
		gapplication->add_action(opendialog);

		auto fullscreen = Gio::SimpleAction::create("fullscreen");
		fullscreen->signal_activate().connect( sigc::mem_fun(*this, &Application::on_fullscreen) );
		fullscreen->set_enabled(true);
		gapplication->add_action(fullscreen);

		for (auto board : CATALOG_BOARDS) {
			Glib::Variant<bool> board_default;
			Glib::ustring action_name = Glib::ustring::compose("toggle_board_%1", board);
			if (settings) {
				Glib::ustring settings_key;
				settings_key = Glib::ustring::compose("board-%1", board);
				settings->get_value(settings_key, board_default);
			} else {
				board_default = Glib::Variant<bool>::create(false);
			}

			auto cboard_toggle = g_simple_action_new_stateful(action_name.c_str(),
			                                                  nullptr,
			                                                  board_default.gobj());
			auto board_toggle = Glib::wrap(cboard_toggle);

			board_toggle->signal_activate().connect( sigc::bind(sigc::mem_fun(*this, &Application::on_board_toggle), board) );
			board_toggle->set_enabled(true);
			gapplication->add_action(board_toggle);
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

			gapplication->set_app_menu(menu);
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
		dialog->add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
		dialog->set_default_response(Gtk::RESPONSE_OK);
		entry->set_activates_default(true);
		box->add(*label);
		box->add(*entry);
		box->show_all();
		auto resp = dialog->run();
		if ( resp == Gtk::RESPONSE_OK ) {
			auto parameter = Glib::Variant<Glib::ustring>::create(entry->get_text());
			gapplication->activate_action("open_thread", parameter);
		}
		dialog->hide();
		delete dialog;
	}

	static bool validate_thread_url(Glib::ustring &url) {
		if ( url.find("4chan.org") != url.npos ) {
			if ( auto pos = url.find("https") != url.npos ) {
				url.erase(pos + 3, 1);
			}
			
			if (url.find("http://") != url.npos)
				return true;
		}

		return false;
	}

	void Application::on_open_thread(const Glib::VariantBase& parameter) {
		Glib::Variant<Glib::ustring> vurl;
		vurl = Glib::VariantBase::cast_dynamic< Glib::Variant<Glib::ustring> >(parameter);
		Glib::ustring url = vurl.get();

		if (validate_thread_url(url)) {
			auto t = Thread::create(url);

			if ( thread_map.count( t->id ) == 0 ) {
				auto tv = Gtk::manage(new ThreadView(t, settings));
				tv->signal_closed.connect( sigc::mem_fun(*this, &Application::on_thread_closed) );

				notebook->append_page(*tv,
				                      *(tv->get_tab_label()));
				notebook->set_tab_reorderable(*tv, true);
				notebook->set_tab_detachable(*tv, false);

				threads.push_back(url);
				std::sort(threads.begin(),
				          threads.end());

				if (settings) {
					settings->set_string_array("threads", threads);
				}

				thread_map.insert({t->id, tv});
				manager.add_thread(t);
				manager.update_threads();
			}
		}
	}

	void Application::on_fullscreen(const Glib::VariantBase&) {
		static bool is_fullscreen = false;

		if (is_fullscreen) {
			window->unfullscreen();
			is_fullscreen = false;
		} else {
			window->fullscreen();
			is_fullscreen = true;
		}
	}

	void Application::on_board_toggle(const Glib::VariantBase &v,
	                                  const std::string &board) {
		const std::string settings_key("board-" + board);
		const bool action_from_menu = v.gobj() == nullptr;
		bool toggle_board = false;
		Glib::Variant<bool> new_setting;

		if ( action_from_menu ) {
			// There was no input, so we toggle the old setting
			if (settings) {
				toggle_board = !settings->get_boolean(settings_key);
			} else {
				auto lambda = [&board](const std::string &catalog_board) {
					return (catalog_board.compare(board) == 0);
				};
				toggle_board = ! manager.for_each_catalog_board(lambda);
			}
		} else {
			auto b = Glib::VariantBase::cast_dynamic<Glib::Variant<bool> >(v);
			toggle_board = b.get();
		}

		new_setting = Glib::Variant<bool>::create( toggle_board );
		if (settings) {
			settings->set_value(settings_key, new_setting);
		}

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
		Glib::ustring label = Glib::ustring::compose("/%1/", board);
		board_combobox->append(label);
		if (active_board.size() > 0 &&
		    active_board.find(label) != active_board.npos)
			board_combobox->set_active_text(label);
		return true;
	}


	/*
	 * Handle ThreadView closures
	 */
	void Application::on_thread_closed(const gint64 id) {
		auto iter = thread_map.find(id);
		if ( iter != thread_map.end() ) {

			ThreadView *tv = iter->second;
			notebook->remove_page(*tv);

			thread_map.erase(iter);
			remove_thread(id);
		}
	}

	
	/*
	 * Removes thread from manager. This happens on 404s and
	 * ThreadView closures.
	 */
	void Application::remove_thread(const gint64 id) {
		manager.remove_thread(id);

		gchar* sid = g_strdup_printf("/res/%" G_GINT64_FORMAT, id);

		threads.erase(std::remove_if(threads.begin(),
		                             threads.end(),
		                             [&sid](const Glib::ustring &thread) {
			                             return g_str_has_suffix(thread.c_str(), sid);
		                             }),
		              threads.end());

		if (settings) {
			settings->set_string_array("threads", threads);
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
		summary_view->add_events(Gdk::BUTTON_PRESS_MASK);
		if (board_combobox->get_active_row_number() == -1)
			return;
		const std::string active_board = get_board_from_fancy(board_combobox->get_active_text());


		while (manager.is_updated_catalog()) {
			auto board = manager.pop_updated_catalog_board();
			if (board.size() == active_board.size() &&
			    board.find(active_board) != board.npos) {
				refresh_catalog_view();
			}
		}
	}


	void Application::on_catalog_board_change() {
		model->clear();
		refresh_catalog_view();
	}

	void Application::on_catalog_image(const Glib::RefPtr<Gdk::PixbufLoader> &loader,
	                                   Glib::RefPtr<ThreadSummary> thread) {
		if (loader) {
			const gint64 match_id = thread->get_id();
			const Gtk::TreeModelColumn<gint64> column = thread_summary_columns.id;
			auto children = model->children();
			auto iter = std::find_if(children.begin(),
			                         children.end(),
			                         [&match_id, &column](const Gtk::TreeRow &row) {
				                         return match_id == row.get_value(column);
			                         });

			if (iter != children.end()) {
				auto pixbuf = loader->get_pixbuf();
				horizon_thread_summary_set_thumb_pixbuf(thread->gobj(),
				                                        pixbuf->gobj());
				iter->set_value(thread_summary_columns.thumb,
				                pixbuf);
				iter->set_value(thread_summary_columns.thread_summary,
				                thread);
			}
		} else {
			std::cerr << "Warning: CatalogView got invalid PixbufLoader" << std::endl;
		}
	}

	void Application::refresh_catalog_view() {
		if (board_combobox->get_active_row_number() == -1)
			return;
		const std::string active_board = get_board_from_fancy(board_combobox->get_active_text());
		auto ifetcher = ImageFetcher::get(CATALOG);

		try {
			auto catalog = manager.get_catalog(active_board);
			std::map<gint64, Glib::RefPtr<ThreadSummary> > summary_map;
			std::transform(catalog.begin(),
			               catalog.end(),
			               std::inserter(summary_map, summary_map.end()),
			               [](const Glib::RefPtr<ThreadSummary> &summary) {
				               return std::make_pair(summary->get_id(), summary);
			               });

			if ( model ) {
				auto children = model->children();
				std::vector<std::pair<gint64, Gtk::TreeRow> > row_map;
				row_map.reserve(children.size());
				auto column_id = thread_summary_columns.id;
				std::transform(children.begin(),
				               children.end(),
				               std::back_inserter(row_map),
				               [&column_id](const Gtk::TreeRow &row) {
					               return std::make_pair(row.get_value(column_id), row);
				               });
				std::map<gint64, Gtk::TreeRow> erase_map;
				auto partition_iter = std::partition(row_map.begin(),
				                                     row_map.end(),
				                                     [&summary_map](const std::pair<gint64, Gtk::TreeRow> &pair) {
					                                     const gint64 id = pair.first;
					                                     bool row_in_catalog = summary_map.count(id) > 0;
					                                     return row_in_catalog;
				                                     });
				auto partition_iter_copy = partition_iter;
				std::copy(partition_iter, row_map.end(), std::inserter(erase_map, erase_map.end()));
				row_map.erase(partition_iter_copy, row_map.end());

				int update_count = 0;
				for ( auto pair : row_map ) {
					// These are all the rows that can be updated
					Gtk::TreeRow row = pair.second;
					auto match_iter = summary_map.find(pair.first);
					if (match_iter == summary_map.end())
						g_error("Can't find the row to be updated");
					auto thread = match_iter->second;
					auto pixbuf = row.get_value(thread_summary_columns.thumb);
					if (pixbuf) {
						horizon_thread_summary_set_thumb_pixbuf(thread->gobj(),
						                                        pixbuf->gobj());
					}
					row.set_value(thread_summary_columns.reply_count,
					                             thread->get_reply_count());
					row.set_value(thread_summary_columns.image_count,
					                             thread->get_image_count());
					row.set_value(thread_summary_columns.ppm,
					              static_cast<float>(thread->get_reply_count()) /
					              static_cast<float>(Glib::DateTime::
					                                 create_now_utc().to_unix() -
					                                 thread->get_unix_date()));
					row.set_value(thread_summary_columns.thread_summary,
					                             thread);
					summary_map.erase(match_iter);
					update_count++;
				}
				int new_count = 0;
				// summary_map now contains only new summaries
				for ( auto pair : summary_map ) {
					auto thread = pair.second;
					auto iter = model->append();
					auto path = model->get_path(iter);

					auto post = thread->get_proxy_post();
					auto cb = std::bind(&Application::on_catalog_image, this,
					                    std::placeholders::_1,
					                    thread);
					ifetcher->download(post, cb, canceller);

					iter->set_value(thread_summary_columns.thread_summary,
					                thread);
					iter->set_value(thread_summary_columns.teaser,
					                Glib::ustring(thread->get_teaser()));
					iter->set_value(thread_summary_columns.url,
					                Glib::ustring(thread->get_url()));
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
					new_count++;
				}
				int erase_count = 0;
				for ( auto pair : erase_map ) {
					// The Gtk::TreeRow in the pair is now invalid
					const gint64 id_to_erase = pair.first;
					auto slot = sigc::bind(sigc::mem_fun(*this, &Application::erase_iter_if_match_id), id_to_erase);
					model->foreach_iter(slot);
					erase_count++;
				}

				std::cerr << "Catalog updated. " << new_count << " new, " 
				          << update_count << " updated, " << erase_count 
				          << " expired." << std::endl;
			} else {
				g_error("Summary model not created");
			}
		} catch (std::range_error e) {
			return;
		}
	}

	bool Application::erase_iter_if_match_id(const Gtk::TreeModel::iterator& iter, const gint64 id) {
		gint64 row_id = iter->get_value(thread_summary_columns.id);
		if (row_id == id) {
			model->erase(iter);
			return true;
		} else {
			return false;
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
					g_warning("Thread %" G_GINT64_FORMAT " not in application.thread_map", tid);
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
			if ( summary_view->get_bin_window() == Glib::wrap(eb->window, true) ) {
				Gtk::TreeModel::Path path;
				Gtk::TreeViewColumn *column;
				int cell_x, cell_y;

				summary_view->get_path_at_pos(static_cast<int>(eb->x),
				                              static_cast<int>(eb->y),
				                              path, column, cell_x, cell_y);


				auto iter = model->get_iter(path);
				auto val = iter->get_value(thread_summary_columns.url);
				auto variant = Glib::Variant<Glib::ustring>::create(val);

				gapplication->activate_action("open_thread", variant);
				std::cerr << "Opening thread " << val << std::endl;
			} 
		}

		return true;
	}

	bool Application::on_search_equal(const Glib::RefPtr<Gtk::TreeModel>&,
	                                  int,
	                                  const Glib::ustring& key,
	                                  const Gtk::TreeModel::iterator& iter) {
		auto val = iter->get_value(thread_summary_columns.teaser);
		if ( val.casefold().find(key.casefold()) == val.npos ) {
			return true;
		} else {
			return false;
		}
	}

	Application::~Application() {
		canceller->cancel();
		manager_alarm.disconnect();
		summary_alarm.disconnect();
	}


	void Application::run(int argc, char *argv[]) {
		gapplication->run(argc, argv);
	}

	Application::Application(const Glib::ustring &appid) :
		canceller(new Canceller())
	{
		gapplication = Gtk::Application::create(appid);
		gapplication->signal_activate().connect(sigc::mem_fun(*this, &Application::on_activate));
		gapplication->signal_startup().connect(sigc::mem_fun(*this, &Application::on_startup));

		Notifier::init();
	}

}

