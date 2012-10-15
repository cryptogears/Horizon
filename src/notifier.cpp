#include "notifier.hpp"
#include <iostream>
#include <map>



namespace Horizon {

	std::shared_ptr<Notifier> Notifier::getNotifier() {
		static std::shared_ptr<Notifier> notifier = std::shared_ptr<Notifier>(new Notifier());
		return notifier;
	}

	void Notifier::init() {
		getNotifier();
	}

	Notifier::Notifier() {
		auto slot = sigc::mem_fun(this, &Notifier::on_proxy_created);

		Gio::DBus::Proxy::create_for_bus(Gio::DBus::BUS_TYPE_SESSION,
		                                 "org.freedesktop.Notifications",
		                                 "/org/freedesktop/Notifications",
		                                 "org.freedesktop.Notifications",
		                                 slot);

		GError *error = nullptr;
		GdkPixbuf *icon = gdk_pixbuf_new_from_resource("/com/talisein/fourchan/native/gtk/4chan-icon.png", &error);
		if (error) {
			g_error("Unable to create icon from resource: %s", error->message);
		} else {
			default_icon = Glib::wrap(icon);
		}
		
	}

	void Notifier::on_proxy_created(Glib::RefPtr<Gio::AsyncResult> &result) {
		try {
			proxy = Gio::DBus::Proxy::create_for_bus_finish(result);
		} catch (Glib::Error e) {
			g_error("Could not create DBus Proxy: %s", e.what().c_str());
		}
	}

	void Notifier::on_notification_sent(Glib::RefPtr<Gio::AsyncResult> &result) {
		try {
			proxy->call_finish(result);
		} catch (Glib::Error e) {
			std::cerr << "Error sending notification: " << e.what() << std::endl;
		}
	}

	Notifier::~Notifier() {
	}

	void Notifier::send_notification(Glib::VariantContainerBase notification) {
		if (proxy) {
			const auto slot = sigc::mem_fun(*this, &Notifier::on_notification_sent);
			proxy->call("Notify",
			            slot,
			            notification);
		} else {
			g_error("Proxy wasn't ready for sending a notification!");
		}
	}

	void Notifier::notify(const gint64 id,
	                      const std::string &summary,
	                      const std::string &body,
	                      const std::string &icon_url,
	                      const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {

		std::vector<Glib::ustring> actions;
		std::map<Glib::ustring, Glib::VariantBase> hints;

		actions.push_back("win.reply");
		actions.push_back("Reply");

		if (pixbuf) {
			GdkPixbuf *scaled_pixbuf = nullptr;
			if (pixbuf->get_width() > 125 || pixbuf->get_height() > 125) {
				scaled_pixbuf = scale_pixbuf(pixbuf->gobj());
			} else {
				scaled_pixbuf = pixbuf->gobj_copy();
			}

			Glib::VariantContainerBase image_data = get_variant_from_pixbuf(scaled_pixbuf);
			hints.insert({"image-data", image_data});
			g_object_unref(scaled_pixbuf);
		} else {
			//Glib::VariantContainerBase image_data = get_variant_from_pixbuf(default_icon->gobj());
			//hints.insert({"image-data", image_data});
		}

		auto vresident = Glib::Variant<bool>::create(true);
		//hints.insert({"resident", vresident});
		auto vcategory = Glib::Variant<Glib::ustring>::create("im.received");
		hints.insert({"category", vcategory});
		
		auto vappname = Glib::Variant<Glib::ustring>::create("Horizon");
		auto vid = Glib::Variant<guint32>::create(static_cast<guint32>(id));
		auto viurl = Glib::Variant<Glib::ustring>::create(icon_url);
		auto vsummary = Glib::Variant<Glib::ustring>::create(summary);
		auto vbody = Glib::Variant<Glib::ustring>::create(body);
		auto vactions =  Glib::Variant<std::vector<Glib::ustring> >::create(actions);
		auto vhints = Glib::Variant<std::map<Glib::ustring, Glib::VariantBase> >::create(hints);
		auto vtimeout = Glib::Variant<gint32>::create(-1);
		auto vnote = Glib::VariantContainerBase::create_tuple({vappname,
					vid,
					viurl,
					vsummary,
					vbody,
					vactions,
					vhints,
					vtimeout});
		send_notification(vnote);
	}

	GdkPixbuf* Notifier::scale_pixbuf(const GdkPixbuf *pixbuf) {
		const double target_width = 125.;
		const double target_height = 125.;
		double width = static_cast<double>(gdk_pixbuf_get_width(pixbuf));
		double height = static_cast<double>(gdk_pixbuf_get_height(pixbuf));
		gint new_width;
		gint new_height;
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
		
		return gdk_pixbuf_scale_simple(pixbuf,
		                               new_width,
		                               new_height,
		                               GDK_INTERP_BILINEAR);
	}

	Glib::VariantContainerBase Notifier::get_variant_from_pixbuf(GdkPixbuf *pixbuf) {
		gint            width;
		gint            height;
		gint            rowstride;
		gint            n_channels;
		gint            bits_per_sample;
		guchar         *image;
		gboolean        has_alpha;
		gsize           image_len;

		width = gdk_pixbuf_get_width(pixbuf);
		height = gdk_pixbuf_get_height(pixbuf);
		rowstride = gdk_pixbuf_get_rowstride(pixbuf);
		n_channels = gdk_pixbuf_get_n_channels(pixbuf);
		bits_per_sample = gdk_pixbuf_get_bits_per_sample(pixbuf);
		image = gdk_pixbuf_get_pixels(pixbuf);
		has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
		image_len = (height - 1) * rowstride + width *
			((n_channels * bits_per_sample + 7) / 8);

		auto vwidth = Glib::Variant<gint32>::create(width);
		auto vheight = Glib::Variant<gint32>::create(height);
		auto vrowstride = Glib::Variant<gint32>::create(rowstride);
		auto valpha = Glib::Variant<bool>::create(has_alpha);
		auto vbits_per_sample = Glib::Variant<gint32>::create(bits_per_sample);
		auto vchannels = Glib::Variant<gint32>::create(n_channels);
		std::vector<guchar> data;
		data.reserve(image_len);
		for ( gsize i = 0; i < image_len; i++ ) {
			data.push_back(image[i]);
		}

		auto vdata = Glib::Variant<std::vector<guchar> >::create(data);
		auto vimage_data = Glib::VariantContainerBase::create_tuple({vwidth,
					vheight,
					vrowstride,
					valpha,
					vbits_per_sample,
					vchannels,
					vdata});

		return vimage_data;
	}
}
