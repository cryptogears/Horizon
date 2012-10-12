#ifndef IMAGE_CACHE_HPP
#define IMAGE_CACHE_HPP
#include <libev/ev++.h>
#include <vector>
#include <set>
#include <deque>
#include <memory>
#include <glibmm/variant.h>
#include <glibmm/threads.h>
#include <giomm/file.h>
#include <giomm/memoryinputstream.h>
#include <gdkmm/pixbufloader.h>
#include "thread.hpp"

namespace Horizon {

	class ImageData {
	public:
		ImageData() = delete;
		ImageData(const ImageData&) = delete;
		ImageData& operator=(const ImageData&) = delete;
		~ImageData();
		static std::shared_ptr<ImageData> create(const guint32 version, const Glib::VariantContainerBase &);
		static std::shared_ptr<ImageData> create(const Glib::RefPtr<Post> &post);

	private:
		ImageData(const guint32 version, const Glib::VariantContainerBase &);
		ImageData(const Glib::RefPtr<Post> &post);

	public:
		void update(const Glib::RefPtr<Post> &post);

		Glib::VariantContainerBase get_variant() const;
		Glib::ustring get_uri(bool thumb) const;

		gsize size;
		std::string md5;
		std::string ext;
		std::set<Glib::ustring> boards;
		std::set<Glib::ustring> tags;
		std::set<std::string> original_filenames; // Potentially UNIX + milli
		std::set<std::string> poster_names;
		std::set<gint64> posted_unix_dates;
		guint16 num_spoiler;
		guint16 num_deleted;
		bool have_thumbnail;
		bool have_image;
	};

	class ImageCache {
	public:
		static std::shared_ptr<ImageCache> get();
		~ImageCache();

		bool has_thumb(const Glib::RefPtr<Post> &post);
		bool has_image(const Glib::RefPtr<Post> &post);

		void get_thumb_async(const Glib::RefPtr<Post> &post,
		                     std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback);
		void get_image_async(const Glib::RefPtr<Post> &post,
		                     std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback);

		void write_thumb_async(const Glib::RefPtr<Post> &post,
		                       Glib::RefPtr<Gio::MemoryInputStream> &istream);
		void write_image_async(const Glib::RefPtr<Post> &post,
		                       Glib::RefPtr<Gio::MemoryInputStream> &istream);

		void flush();

	protected:
		ImageCache();

	private:
		mutable Glib::Threads::Mutex map_lock;
		std::map<std::string, std::shared_ptr<ImageData> > images;

		mutable Glib::Threads::Mutex thumb_write_queue_lock;
		std::deque< std::pair< Glib::RefPtr<Post>,
		                       Glib::RefPtr<Gio::MemoryInputStream> > > thumb_write_queue;
		void write(const Glib::RefPtr<Post> &,
		           Glib::RefPtr<Gio::MemoryInputStream>,
		           bool write_thumb);
		                 
		mutable Glib::Threads::Mutex image_write_queue_lock;
		std::deque< std::pair< Glib::RefPtr<Post>,
		                       Glib::RefPtr<Gio::MemoryInputStream> > > image_write_queue;

		mutable Glib::Threads::Mutex thumb_read_queue_lock;
		std::deque< std::pair< Glib::RefPtr<Post>,
		                       std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> > > thumb_read_queue;
		void read_thumb(const Glib::RefPtr<Post>&,
		                std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)>);

		mutable Glib::Threads::Mutex image_read_queue_lock;
		std::deque< std::pair< Glib::RefPtr<Post>,
		                       std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> > > image_read_queue;
		void read_image(const Glib::RefPtr<Post> &,
		                std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)>);

		void read(const std::shared_ptr<ImageData> &,
		          std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)>,
		          bool read_thumb);

		void             read_from_disk();

		Glib::Threads::Thread *ev_thread;
		ev::dynamic_loop ev_loop;
		void             loop();

		ev::async        kill_loop_w;
		void             on_kill_loop_w(ev::async &w, int);

		ev::async        write_queue_w;
		void             on_write_queue_w(ev::async &w, int);

		ev::async        read_queue_w;
		void             on_read_queue_w(ev::async &w, int);

		ev::async        flush_w;
		void             on_flush_w(ev::async &, int);
	};

	const std::string CACHE_FILENAME = "horizon-cache.dat";
	const guint32 CACHE_FILE_VERSION = 1;
	const std::string CACHE_VERSION_1_TYPE = "(tayayasasaayaayaxqqbb)";
	const std::string CACHE_VERSION_1_ARRAYTYPE = "a(tayayasasaayaayaxqqbb)";
}


#endif
