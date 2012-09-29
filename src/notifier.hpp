#ifndef NOTIFIER_HPP
#define NOTIFIER_HPP

#include <memory>
#include <giomm/dbusproxy.h>
#include <glibmm/variant.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/threads.h>

namespace Horizon {

	class Notifier {
	public:
		static std::shared_ptr<Notifier> getNotifier();
		static void init();

		~Notifier();

		void notify(const gint64 id,
		            const std::string &summary,
		            const std::string &body,
		            const std::string &icon_url,
		            const Glib::RefPtr<Gdk::Pixbuf> &pixbuf);

	protected:
		Notifier();

	private:
		GdkPixbuf* scale_pixbuf(const GdkPixbuf *pixbuf);
		Glib::VariantContainerBase get_variant_from_pixbuf(GdkPixbuf *pixbuf);

		Glib::RefPtr<Gio::DBus::Proxy> proxy;

		void send_notification(Glib::VariantContainerBase notification);
		void on_proxy_created(Glib::RefPtr<Gio::AsyncResult> &result);
		void on_notification_sent(Glib::RefPtr<Gio::AsyncResult> &result);
	};

}


#endif
