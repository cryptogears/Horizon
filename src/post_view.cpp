#include "post_view.hpp"

#include <iostream>
#include <glibmm/convert.h>
#include <gtkmm/viewport.h>
#include <gtkmm/scrolledwindow.h>
#include "horizon-resources.h"
#include <giomm/file.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>
#include <glibmm/markup.h>
#include <glibmm/datetime.h>
#include <gtkmm/cssprovider.h>
#include <libnotify/notify.h>
#include <ratio>

namespace Horizon {

	PostView::PostView(const Glib::RefPtr<Post> &in, const bool donotify) :
		Gtk::Grid(),
		comment(),
		comment_grid(),
		vadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		hadjust(Gtk::Adjustment::create(0., 0., 1., .1, 0.9, 1.)),
		comment_viewport(hadjust, vadjust),
		post(in),
		ifetcher(ImageFetcher::get()),
		notify(donotify),
		scaled_width(-1)
	{
		set_name("postview");
		set_orientation(Gtk::ORIENTATION_VERTICAL);
		
		auto ngrid = Gtk::manage(new Gtk::Grid());
		ngrid->set_name("posttopgrid");
		ngrid->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		ngrid->set_column_spacing(5);

		auto label = Gtk::manage(new Gtk::Label(post->get_subject()));
		label->set_name("subject");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post->get_name()));
		label->set_name("name");
		label->set_hexpand(false);
		label->set_ellipsize(Pango::ELLIPSIZE_END);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label(post->get_time_str()));
		label->set_name("time");
		label->set_hexpand(false);
		ngrid->add(*label);

		label = Gtk::manage(new Gtk::Label("No. " + post->get_number()));
		label->set_hexpand(false);

		label->set_justify(Gtk::JUSTIFY_LEFT);
		ngrid->add(*label);

		add(*ngrid);
		
		// Image info
		if ( post->has_image() ) {
			ngrid = Gtk::manage(new Gtk::Grid());
			ngrid->set_name("imageinfogrid");
			ngrid->set_column_spacing(5);

			std::string filename = post->get_original_filename() + post->get_image_ext();

			label = Gtk::manage(new Gtk::Label(filename));
			label->set_name("originalfilename");
			label->set_hexpand(false);
			label->set_ellipsize(Pango::ELLIPSIZE_END);
			ngrid->add(*label);

			const gint64 original_id = g_ascii_strtoll(post->get_original_filename().c_str(), NULL, 10);
			const gint64 original_time  = original_id / 1000;
			if (original_time > 1000000000 && original_time < 10000000000) {
				Glib::DateTime dtime = Glib::DateTime::create_now_local(original_time);
				label = Gtk::manage(new Gtk::Label(dtime.format("%B %-e, %Y")));
				label->set_hexpand(false);
				label->set_name("imagenameinfo");
				ngrid->add(*label);
			} 

			std::stringstream imgdims;
			imgdims << " (" << post->get_fsize() / 1000 
			        << " KB, " << post->get_width() 
			        << "x" << post->get_height() << ") ";

			label = Gtk::manage(new Gtk::Label(imgdims.str()));
			label->set_hexpand(false);
			label->set_justify(Gtk::JUSTIFY_LEFT);
			label->set_name("imagenameinfo");
			ngrid->add(*label);
			add(*ngrid);
		}

		sax = (htmlSAXHandlerPtr) calloc(1, sizeof(xmlSAXHandler));
		sax->startElement = &startElement;
		sax->characters = &onCharacters;
		sax->serror = &on_xmlError;
		sax->initialized = XML_SAX2_MAGIC;

		built_string.clear();
		parseComment();

		comment.set_name("comment");
		comment.set_markup(built_string);
		comment.set_selectable(true);
		comment.set_valign(Gtk::ALIGN_START);
		comment.set_halign(Gtk::ALIGN_START);
		comment.set_justify(Gtk::JUSTIFY_LEFT);
		comment.set_line_wrap(true);
		comment.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);

		comment_grid.set_name("commentgrid");
		comment_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
		comment_grid.add(comment);
		comment_viewport.set_name("commentview");
		comment_viewport.add(comment_grid);
		comment_viewport.set_hexpand(true);
		add(comment_viewport);

		image_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
		image_box.add(image);
		image_window.add(image_box);
		image_window.set_name("imagewindow");
		image_box.set_events(Gdk::BUTTON_PRESS_MASK);
		image_box.set_visible_window(false);
		image_box.signal_button_press_event().connect( sigc::mem_fun(*this, &PostView::on_image_click) );
		image_state = NONE;
		image.set_name("image");
		image.set_halign(Gtk::ALIGN_START);
		image.set_valign(Gtk::ALIGN_START);
		
		image.signal_draw().connect( sigc::mem_fun(*this, &PostView::on_image_draw) );
		if ( post->has_image() ) {
			image_state = THUMBNAIL;

			if (post->is_gif()) {
				if (notify) {
					do_notify();
				}
				if ( !ifetcher->has_animation(post->get_hash()) ) {
					image_connection = ifetcher->signal_image_ready.connect(sigc::mem_fun(*this, &PostView::on_image_ready));
					ifetcher->download_image(post->get_hash(),
					                         post->get_image_url(),
					                         post->get_image_ext());
				} else {
					on_image_ready(post->get_hash());
				}
			} else {
				if ( ! ifetcher->has_thumb(post->get_hash()) ) {
					thumb_connection = ifetcher->signal_thumb_ready.connect(sigc::mem_fun(*this, &PostView::on_thumb_ready));
					ifetcher->download_thumb(post->get_hash(),
					                         post->get_thumb_url());
				} else {
					on_thumb_ready(post->get_hash());
				}
			}
		} else {
			// We notify now because we don't need to wait for the image
			if (notify) { 
				do_notify();
			}
		}
		
		//comment_viewport.signal_realize().connect( sigc::mem_fun(*this, &PostView::on_postview_show) );
		//comment_viewport.signal_map_event().connect( sigc::mem_fun(*this, &PostView::on_my_mapped) );
	}

	bool PostView::on_image_draw(const Cairo::RefPtr<Cairo::Context> &ctx) {
		if ( image_state == EXPAND ) {
			if ( unscaled_animation ) {
				// gif
			} else {
				// Not a gif
				double x1, x2, y1, y2;
				ctx->get_clip_extents(x1, y1, x2, y2);
				int width = static_cast<int>(x2 - x1);

				if (scaled_width != width)
					set_new_scaled_image(width);
				else if (comment_grid.get_allocated_width() > width) {
					std::cerr << "Comment grid is bigger by " << comment_grid.get_allocated_width() - width << std::endl;
				} else {
					std::cerr << "Comment grid is " << comment_grid.get_allocated_width() << " width: " << width << std::endl;
				}
			}
		}
		return false;
	}

	void PostView::set_new_scaled_image(const int width) {
		int full_width = post->get_width();
		int full_height = post->get_height();
		
		if (width >= full_width) {
			if ( unscaled_image != image.get_pixbuf() ) {
				image.set(unscaled_image);
				image_window.set_size_request(full_width, full_height);
				scaled_width = -1;
			}
		} else {
			if (unscaled_image && 
			    (width != scaled_width || scaled_width == -1) ) {
				scaled_width = width;
				float scale = static_cast<float>(width)
					/ static_cast<float>(full_width);
				int new_width = static_cast<int>(scale*full_width);
				int new_height = static_cast<int>(scale*full_height);
				scaled_height = new_height;
				scaled_image = unscaled_image->scale_simple(new_width,
				                                            new_height,
				                                            Gdk::INTERP_BILINEAR);
				image.set(scaled_image);
				image_window.set_size_request(new_width, new_height);
			} else {
				if ( width == scaled_width &&
				     scaled_image != image.get_pixbuf() ) {
					image.set(scaled_image);
					image_window.set_size_request(scaled_width,
					                              scaled_height);
				}
					
			}
		}
	}
	
	bool PostView::on_image_click( GdkEventButton* btn ) {
		switch (image_state) {
		case NONE:
			break;
		case THUMBNAIL:
			if (!unscaled_image) {
				if (!image_connection.connected()) {
					image_connection = ifetcher->signal_image_ready.connect(sigc::mem_fun(*this, &PostView::on_image_ready));
					ifetcher->download_image(post->get_hash(),
					                         post->get_image_url(),
					                         post->get_image_ext());
				}
			} else {
				std::cerr << "EXPANDing: To width " << comment_grid.get_allocated_width() << std::endl;
				std::cerr << "Scaled_width = " << scaled_width << std::endl;
				image_state = EXPAND;
				set_new_scaled_image( comment_grid.get_allocated_width() );

				comment_grid.remove(comment);
				comment_grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
				comment_grid.add(comment);
			}
			break;
		case EXPAND:
			image_state = FULL;
			// For now fall through, we don't have a 'Full' implementation
		case FULL: 
			image_state = THUMBNAIL;
			if (thumbnail_image) {
				image.set(thumbnail_image);
				image_window.set_size_request(post->get_thumb_width(), post->get_thumb_height());
				comment_grid.remove(comment);
				comment_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
				comment_grid.add(comment);
			}
			
			break;
		default:
			break;
		}
	}

	bool PostView::on_my_mapped(GdkEventAny* const&) {
		auto context = comment_viewport.get_style_context();
		auto window = comment_viewport.get_window();

		/*
		context->notify_state_change( window,
		                              0,
		                              Gtk::STATE_ACTIVE,
		                              false );
		context->notify_state_change( window,
		                              0,
		                              Gtk::STATE_ACTIVE,
		                              true );
		*/

		comment_viewport.set_state_flags(Gtk::STATE_FLAG_ACTIVE, true);

		return false;
	}

	void PostView::on_postview_show() {
		auto vpwin = comment_viewport.get_window();
		vpwin->set_events(Gdk::STRUCTURE_MASK ^ vpwin->get_events());

	}

	PostView::~PostView() {
		free(sax);
	}

	void PostView::refresh(const Glib::RefPtr<Post> &in) {
		post = in;
	}

	const bool PostView::should_notify() const {
		return notify;
	}

	const bool PostView::should_notify(const bool in) {
		return notify = in;
	}

	Glib::RefPtr<Gio::DBus::Proxy> PostView::get_dbus_proxy() {
		static Glib::RefPtr<Gio::DBus::Proxy> proxy = Gio::DBus::Proxy::create_for_bus_sync(
			              Gio::DBus::BUS_TYPE_SESSION,
			              "org.freedesktop.Notifications",
			              "/org/freedesktop/Notifications",
			              "org.freedesktop.Notifications",
			              Glib::RefPtr< Gio::DBus::InterfaceInfo >(),
			              Gio::DBus::PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES);

		return proxy;
	}
	
	static GVariant* get_variant_from_pixbuf(GdkPixbuf *pixbuf) {
		gint            width;
        gint            height;
        gint            rowstride;
        gint            n_channels;
        gint            bits_per_sample;
        guchar         *image;
        gboolean        has_alpha;
        gsize           image_len;
        GVariant       *value;

        width = gdk_pixbuf_get_width(pixbuf);
        height = gdk_pixbuf_get_height(pixbuf);
        rowstride = gdk_pixbuf_get_rowstride(pixbuf);
        n_channels = gdk_pixbuf_get_n_channels(pixbuf);
        bits_per_sample = gdk_pixbuf_get_bits_per_sample(pixbuf);
        image = gdk_pixbuf_get_pixels(pixbuf);
        has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

        image_len = (height - 1) * rowstride + width *
	        ((n_channels * bits_per_sample + 7) / 8);

        value = g_variant_new ("(iiibii@ay)",
                               width,
                               height,
                               rowstride,
                               has_alpha,
                               bits_per_sample,
                               n_channels,
                               g_variant_new_from_data (G_VARIANT_TYPE ("ay"),
                                                        image,
                                                        image_len,
                                                        TRUE,
                                                        (GDestroyNotify) g_object_unref,
                                                        g_object_ref (pixbuf)));

        return value;
	}

	void PostView::do_notify(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf ) {
		notify = false;
		std::string summary = "New 4chan post";
		std::string body = built_string;
		if (body.size() == 0)
			body = "";
		GError *error = nullptr;
		GVariantBuilder actions_builder;
		GVariantBuilder hints_builder;

		static GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_resource("/com/talisein/fourchan/native/gtk/4chan-icon.png", &error);
		if (!icon_pixbuf) {
			g_error("Unable to load 4chan icon: %s", error->message);
			g_error_free(error);
			error = nullptr;
		}
		static GVariant *icon = get_variant_from_pixbuf(icon_pixbuf);
		g_variant_builder_init (&actions_builder, G_VARIANT_TYPE ("as")); 
		g_variant_builder_add(&actions_builder, "s", "win.reply");
		g_variant_builder_add(&actions_builder, "s", "Reply");
		
		g_variant_builder_init (&hints_builder, G_VARIANT_TYPE ("a{sv}"));

		if (pixbuf) {
			const double target_width = 145.;
			const double target_height = 140.;
			GdkPixbuf *scaled_image = pixbuf->gobj();
			double width = static_cast<double>(pixbuf->get_width());
			double height = static_cast<double>(pixbuf->get_height());
			gint new_width;
			gint new_height;
			bool do_unref = false;
			double ratio = width / height;
			if ( width > height ) {
				height = (target_width / width) * height;
				width = target_width;
			} else {
				width = ( target_height / height ) * width;
				height = target_height;
			}

			if ( width > target_width ) {
				gdouble shrink_ratio = target_width / width;
				width = target_width;
				height = height * shrink_ratio;
			}
			if ( height > target_height ) {
				gdouble shrink_ratio = target_height / height;
				height = target_height;
				width = width * shrink_ratio;
			}

			new_width = static_cast<gint>(width);
			new_height = static_cast<gint>(height);
			std::cerr << "Scaling to " << new_width << "x" << new_height << std::endl;
			scaled_image = gdk_pixbuf_scale_simple(pixbuf->gobj(), new_width, new_height, GDK_INTERP_HYPER);
			do_unref = true;

			GVariant *image_data = get_variant_from_pixbuf(scaled_image);
			g_variant_builder_add (&hints_builder, "{sv}", "image-data", image_data);
			if (do_unref)
				g_object_unref(scaled_image);
		} 

		GVariant *notification = g_variant_new ("(susssasa{sv}i)",
		                                        "Horizon", // (s) app name
		                                        0, // (u) id
		                                        "", // (s) iconname
		                                        summary.c_str(), // (s) summary
		                                        body.c_str(), // (s) body
		                                        &actions_builder, // (as) actions
		                                        &hints_builder, // a{sv} hints
		                                        -1); // (i) timeout
		
		auto proxy = get_dbus_proxy();

		auto result = g_dbus_proxy_call_sync (proxy->gobj(),
		                                 "Notify",
		                                 notification,
		                                 G_DBUS_CALL_FLAGS_NONE,
		                                 -1,
		                                 NULL,
		                                 &error);
		if ( error ) {
			g_warning("Error sending notification: ", error->message);
		}
	}

	void PostView::on_image_ready(std::string hash) {
		if ( post->get_hash().find(hash) != std::string::npos ) {
			if (image_connection.connected())
				image_connection.disconnect();

			image_state = EXPAND;

			if (post->is_gif()) {
				unscaled_animation = ifetcher->get_animation(hash);

				image.set(unscaled_animation);
				image_window.set_size_request(post->get_width(), post->get_height());
				comment_grid.remove(comment);
				comment_grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
				comment_grid.add(image_window);
				comment_grid.add(comment);
				comment_grid.show_all();

			} else {
				unscaled_image = ifetcher->get_image(hash);
				set_new_scaled_image(comment_grid.get_allocated_width());
				comment_grid.set_orientation(Gtk::ORIENTATION_VERTICAL);
				comment_grid.remove(comment);
				comment_grid.add(comment);
			}
		} else {
			// Signal wasn't for this post
			return;
		}
	}

	void PostView::on_thumb_ready(std::string hash) {
		if ( post->get_hash().find(hash) != std::string::npos ) {
			if (thumb_connection.connected())
				thumb_connection.disconnect();
			try {
				thumbnail_image = ifetcher->get_thumb(hash);
				if (G_UNLIKELY(!thumbnail_image)) {
					g_error("Received pixbuf is null!");
				}
				if (notify) {
					do_notify(thumbnail_image);
				}
				try {
					image.set(thumbnail_image);

					int width = post->get_thumb_width();
					int height = post->get_thumb_height();
					image_window.set_size_request(width, height);

					comment_grid.remove(comment);
					comment_grid.add(image_window);
					comment_grid.add(comment);
					comment_grid.show_all();
				} catch ( ... ) {
					g_warning("Failed to construct image");
				}
			} catch ( Glib::Error e ) {
				g_warning("Error creating image from Pixmap: %s", e.what().c_str());
			}
		}
	}

	void PostView::parseComment() {

		ctxt = htmlCreatePushParserCtxt(sax, this, NULL, 0, NULL,
		                                XML_CHAR_ENCODING_UTF8);
		if (G_UNLIKELY( ctxt == NULL )) {
			g_error("Unable to create libxml2 HTML context!");
		}

		htmlCtxtUseOptions(ctxt, HTML_PARSE_RECOVER );
		htmlParseChunk(ctxt, post->get_comment().c_str(), post->get_comment().size(), 1);
		htmlCtxtReset(ctxt);
		htmlFreeParserCtxt(ctxt);
		ctxt = NULL;
	}

	void startElement(void* user_data,
	                  const xmlChar* name,
	                  const xmlChar** attrs) {
		PostView* pv = static_cast<PostView*>(user_data);

		try {
			Glib::ustring str( reinterpret_cast<const char*>(name) );

			if ( str.find("br") != Glib::ustring::npos ) {
				//pv->comment_iter = pv->comment->insert(pv->comment_iter, "\n");
				pv->built_string.append("\n");
			} else {
				if (false) {
					std::cerr << "Ignoring tag " << name;
					if (attrs != NULL) {
						std::cerr << " attrs:";
						for ( int i = 0; attrs[i] != NULL; i++) {
							std::cerr << " " << attrs[i];
					}
					}
					std::cerr << std::endl;
				}
			}
		} catch (std::exception e) {
			std::cerr << "Error startElement casting to string: " << e.what()
			          << std::endl;
		}
	}

	void onCharacters(void* user_data, const xmlChar* chars, int size) {
		PostView* pv = static_cast<PostView*>(user_data);

		Glib::ustring str;
		std::string locale;
		std::string locale_str;
		Glib::get_charset(locale);
		
		try {
			// TODO: This is way too much work; xmlChar is already in
			// UTF-8, we should be able to stick it right in a
			// ustring. Only problem is everytime I try it barfs.
			locale_str = Glib::convert(std::string(reinterpret_cast<const char*>(chars), size), locale, "UTF-8");
			str = Glib::locale_to_utf8(locale_str);
			
		} catch (Glib::ConvertError e) {
			g_error("Error casting '%s' to Glib::ustring: %s", chars, e.what().c_str());
		}
		
		gchar* cstr = g_markup_escape_text(static_cast<const gchar*>(str.c_str()), -1);
		pv->built_string.append(static_cast<char*>(cstr));
		g_free(cstr);
		//pv->comment_iter = pv->comment->insert(pv->comment_iter, str);
	}

	void on_xmlError(void* user_data, xmlErrorPtr error) {
		Horizon::PostView* postview = static_cast<PostView*>(user_data);
		switch (error->code) {
		case XML_ERR_INVALID_ENCODING:
			std::cerr << "Warning: Invalid UTF-8 encoding. I blame moot. "
			          << "If you're saving by original filename, its going to get"
			          << " ugly. It can't be helped." << std::endl;
			break;
		case XML_ERR_NAME_REQUIRED:
		case XML_ERR_TAG_NAME_MISMATCH:
		case XML_ERR_ENTITYREF_SEMICOL_MISSING:
		case XML_ERR_INVALID_DEC_CHARREF:
		case XML_ERR_LTSLASH_REQUIRED:
		case XML_ERR_GT_REQUIRED:
			// Ignore
			break;
		default:
			if (error->domain == XML_FROM_HTML) {
				std::cerr << "Warning: Got unexpected libxml2 HTML error code "
				          << error->code << ". This is probably ok : " 
				          << error->message;
			} else {
				std::cerr << "Error: Got unexpected libxml2 error code "
				          << error->code << " from domain " << error->domain
				          << " which means: " << error->message << std::endl;
				std::cerr << "\tPlease report this error to the developer."
				          << std::endl;
			}
		}
	}


}

