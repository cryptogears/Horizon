#ifndef THREAD_SUMMARY_HPP
#define THREAD_SUMMARY_HPP
#include <glibmm/object.h>
#include <glibmm/private/object_p.h>
#include <gdkmm/pixbufloader.h>
#include "thread.hpp"
#include "canceller.hpp"

extern "C" {
#include "horizon_thread_summary.h"
}

namespace Horizon {
	class ThreadSummary;

	class ThreadSummary_Class : public Glib::Class {
	public:
		typedef ThreadSummary CppObjectType;
		typedef HorizonThreadSummary BaseObjectType;
		typedef HorizonThreadSummaryClass BaseClassType;
		typedef Glib::Object_Class CppClassParent;
		typedef GObjectClass BaseClassParent;

		friend class ThreadSummary;

		const Glib::Class& init();
		static void class_init_function(void *g_class, void *class_data);
		static Glib::ObjectBase* wrap_new(GObject *object);
	};

	class ThreadSummary : public Glib::Object {
	public:
		typedef ThreadSummary CppObjectType;
		typedef ThreadSummary_Class CppClassType;
		typedef HorizonThreadSummary BaseObjectType;
		typedef HorizonThreadSummaryClass BaseObjectClassType;

	private:
		friend class ThreadSummary_Class;
		static CppClassType thread_summary_class_;

	public:
		virtual ~ThreadSummary();
		ThreadSummary() = delete;
		explicit ThreadSummary(const ThreadSummary&) = delete;
		ThreadSummary& operator=(const ThreadSummary&) = delete;

		static GType get_type()   G_GNUC_CONST;
		static GType get_base_type()  G_GNUC_CONST;

		HorizonThreadSummary* gobj() { return reinterpret_cast<HorizonThreadSummary*>(gobject_); };
		const HorizonThreadSummary* gobj() const { return reinterpret_cast<HorizonThreadSummary*>(gobject_);};
		HorizonThreadSummary* gobj_copy();

	protected:
		explicit ThreadSummary(const Glib::ConstructParams& construct_params);
		explicit ThreadSummary(HorizonThreadSummary* castitem);

	public:
		gint64 get_id() const;
		gint64 get_image_count() const;
		gint64 get_reply_count() const;
		void set_id(gint64 id);
		void set_id(const gchar* sid);
		void set_board(const gchar* board);
		const std::string get_url() const;
		const std::string get_teaser() const;
		const std::string get_hash() const;
		Glib::RefPtr<Gdk::Pixbuf> get_thumb_pixbuf();
		gint64 get_unix_date() const;

		void fetch_thumb();
		Glib::RefPtr<Horizon::Post> get_proxy_post() const;

	private:
		void on_thumb(const Glib::RefPtr<Gdk::PixbufLoader> &loader);
		std::shared_ptr<Canceller> canceller;
	};
}

namespace Glib {
	Glib::RefPtr<Horizon::ThreadSummary> wrap(HorizonThreadSummary* object, bool take_copy = false);
}
#endif
