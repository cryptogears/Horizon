#include <iostream>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <glibmm/uriutils.h>
#include "image_cache.hpp"


namespace Horizon {

	std::shared_ptr<ImageData> ImageData::create(const guint32 version,
	                                             const Glib::VariantContainerBase &variant) {
		auto data = std::shared_ptr<ImageData>(new ImageData(version, variant));
		if (!data)
			g_error("Failed to create image_data.");
		return data;
	}

	std::shared_ptr<ImageData> ImageData::create(const Glib::RefPtr<Post> &post) {
		auto data = std::shared_ptr<ImageData>(new ImageData(post));
		if (!data)
			g_error("Failed to create image_data.");
		return data;
	}

	ImageData::ImageData(const guint32 version, const Glib::VariantContainerBase& variant) {
		gsize num = variant.get_n_children();
		if ( version == 1 ) {
			if (num != 12) {
				g_error("Invalid number of elements in version 1 data: %" G_GSIZE_FORMAT " elements.", num);
			}
			Glib::Variant< gsize >                        vsize;
			Glib::Variant< std::string >                  vmd5;
			Glib::Variant< std::string >                  vext;
			Glib::Variant< std::vector< Glib::ustring > > vboards;
			Glib::Variant< std::vector< Glib::ustring > > vtags;
			Glib::Variant< std::vector< std::string > >   vfilenames;
			Glib::Variant< std::vector< std::string > >   vposters;
			Glib::Variant< std::vector< gint64 > >        vdates;
			Glib::Variant< guint16 >                      vspoilers;
			Glib::Variant< guint16 >                      vdeleted;
			Glib::Variant< bool >                         vhave_thumb;
			Glib::Variant< bool >                         vhave_image;
			variant.get_child(vsize,         0);
			variant.get_child(vmd5,          1);
			variant.get_child(vext,          2);
			variant.get_child(vboards,       3);
			variant.get_child(vtags,         4);
			variant.get_child(vfilenames,    5);
			variant.get_child(vposters,      6);
			variant.get_child(vdates,        7);
			variant.get_child(vspoilers,     8);
			variant.get_child(vdeleted,      9);
			variant.get_child(vhave_thumb,   10);
			variant.get_child(vhave_image,   11);
			std::vector<Glib::ustring> vec_boards = vboards   .get();
			std::vector<Glib::ustring> vec_tags   = vtags     .get();
			std::vector<std::string>   filenames  = vfilenames.get();
			std::vector<std::string>   posters    = vposters  .get();
			std::vector<gint64>        dates      = vdates    .get();

			size                                  = vsize     .get();
			md5                                   = vmd5      .get();
			ext                                   = vext      .get();
			std::copy(vec_boards.rbegin(), vec_boards.rend(), std::inserter(boards,             boards            .begin()));
			std::copy(vec_tags  .rbegin(), vec_tags  .rend(), std::inserter(tags,               tags              .begin()));
			std::copy(filenames .rbegin(), filenames .rend(), std::inserter(original_filenames, original_filenames.begin()));
			std::copy(posters   .rbegin(), posters   .rend(), std::inserter(poster_names,       poster_names      .begin()));
			std::copy(dates     .rbegin(), dates     .rend(), std::inserter(posted_unix_dates,  posted_unix_dates .begin()));
			num_spoiler    = vspoilers.get();
			num_deleted    = vdeleted.get();
			have_thumbnail = vhave_thumb.get();
			have_image     = vhave_image.get();
		} else {
			g_error("Invalid version number for ImageCache data.");
		}
	}

	ImageData::ImageData(const Glib::RefPtr<Post> &post) :
		size(post->get_file_size()),
		md5(post->get_hash()),
		ext(post->get_image_ext()),
		num_spoiler(post->is_spoiler()?1:0),
		num_deleted(0),
		have_thumbnail(false),
		have_image(false)
	{
		boards.insert             ( post->get_board() );
		original_filenames.insert ( post->get_original_filename() );
		poster_names.insert       ( post->get_name() + post->get_tripcode() );
		posted_unix_dates.insert  ( post->get_unix_time() );

		gint64 id = g_ascii_strtoll(post->get_original_filename().c_str(), NULL, 10);
		const gint64 now = Glib::DateTime::create_now_utc().to_unix();
		if ( id > 1000000000 && id < now ) {
			posted_unix_dates.insert ( id );
		} else {
			id = id / 1000;
			if ( id > 1000000000 && id < now ) {
				posted_unix_dates.insert ( id );
			}
		}
	}

	ImageData::~ImageData() {
	}

	void ImageData::update(const Glib::RefPtr<Post> &post) {
		if ( md5.find(post->get_hash()) == md5.npos ) {
			g_error("ImageData::update() called with invalid post. My hash = %s, post hash = %s.",
			        md5.c_str(), post->get_hash().c_str());
		}

		boards.insert            (post->get_board());
		original_filenames.insert(post->get_original_filename());
		poster_names.insert      (post->get_name() + post->get_tripcode());
		auto pair = posted_unix_dates.insert (post->get_unix_time());
		if (pair.second) {// We surely haven't seen this before
			if (post->is_spoiler())
				num_spoiler++;
			if (post->is_deleted()) // FIXME
				num_deleted++;
		}

		gint64 id = g_ascii_strtoll(post->get_original_filename().c_str(), NULL, 10);
		const gint64 now = Glib::DateTime::create_now_utc().to_unix();
		if ( id > 1000000000 && id < now ) {
			posted_unix_dates.insert ( id );
		} else {
			id = id / 1000;
			if ( id > 1000000000 && id < now ) {
				posted_unix_dates.insert ( id );
			}
		}
	}

	Glib::VariantContainerBase ImageData::get_variant() const {
		auto vsize = Glib::Variant<gsize>::create(size);
		auto vmd5 = Glib::Variant<std::string>::create(md5);
		auto vext = Glib::Variant<std::string>::create(ext);
		std::vector<Glib::ustring> vec_boards, vec_tags;
		std::vector<std::string> filenames, posters;
		std::vector<gint64> dates;
		std::copy(boards            .begin(), boards            .end(), std::back_inserter(vec_boards));
		std::copy(tags              .begin(), tags              .end(), std::back_inserter(vec_tags));
		std::copy(original_filenames.begin(), original_filenames.end(), std::back_inserter(filenames));
		std::copy(poster_names      .begin(), poster_names      .end(), std::back_inserter(posters));
		std::copy(posted_unix_dates .begin(), posted_unix_dates .end(), std::back_inserter(dates));
		auto vboards       = Glib::Variant< std::vector<Glib::ustring> >::create(vec_boards);
		auto vtags         = Glib::Variant< std::vector<Glib::ustring> >::create(vec_tags);
		auto vfilenames    = Glib::Variant< std::vector<std::string> >  ::create(filenames);
		auto vposters      = Glib::Variant< std::vector<std::string> >  ::create(posters);
		auto vdates        = Glib::Variant< std::vector<gint64> >       ::create(dates);
		auto vspoilers     = Glib::Variant< guint16 >                   ::create(num_spoiler);
		auto vdeleted      = Glib::Variant< guint16 >                   ::create(num_deleted);
		auto vhave_thumb   = Glib::Variant< bool >                      ::create(have_thumbnail);
		auto vhave_image   = Glib::Variant< bool >                      ::create(have_image);

		return Glib::VariantContainerBase::create_tuple({
				    vsize,          // 0
					vmd5,           // 1
					vext,           // 2
					vboards,        // 3
					vtags,          // 4
					vfilenames,     // 5
					vposters,       // 6
					vdates,         // 7
					vspoilers,      // 8
					vdeleted,       // 9
					vhave_thumb,    // 10
					vhave_image});  // 11
	}

	/*
	 * Requires md5 and ext to be set.
	 */
	Glib::ustring ImageData::get_uri(bool thumb) const {
		std::string type;
		if (thumb)
			type = "thumbs";
		else
			type = "images";

		std::string subdir = Glib::uri_escape_string(md5.substr(0, 2));

		std::string filename;
		if (thumb)
			filename = Glib::uri_escape_string(md5 + ".jpg");
		else
			filename = Glib::uri_escape_string(md5 + ext);

		auto base = Glib::get_user_data_dir();
		std::vector<std::string> elements = {base, "horizon", type, subdir, filename};
		std::string path = Glib::build_filename( elements );

		Glib::ustring ret;
		try {
			ret = Glib::filename_to_uri(path);
		} catch (Glib::ConvertError e) {
			g_warning ("Could not create URI: %s", e.what().c_str());
		}
		return ret;
	}

	std::shared_ptr<ImageCache> ImageCache::get() {
		static std::shared_ptr<ImageCache> singleton = std::shared_ptr<ImageCache>(new ImageCache());

		return singleton;
	}

	bool ImageCache::has_thumb(const Glib::RefPtr<Post> &post) {
		Glib::Threads::Mutex::Lock lock(map_lock);
		auto iter = images.find( post->get_hash() );
		if ( iter != images.end() &&
		     iter->second->have_thumbnail ) {
			return true;
		}
		
		return false;
	}

	bool ImageCache::has_image(const Glib::RefPtr<Post> &post) {
		Glib::Threads::Mutex::Lock lock(map_lock);
		auto iter = images.find( post->get_hash() );
		if ( iter != images.end() &&
		     iter->second->have_image ) {
			return true;
		}
		
		return false;
	}

	void ImageCache::write(const Glib::RefPtr<Post> &post,
	                       Glib::RefPtr<Gio::MemoryInputStream> istream,
	                       bool write_thumb) {
		if (write_thumb && has_thumb(post)) {
			g_warning("write called for thumbnail when we already have a thumbnail");
			return;
		} else if ((!write_thumb) && has_image(post)) {
			g_warning("write called for image when we already have an image");
			return;
		}

		std::shared_ptr<ImageData> image_data;
		{
			Glib::Threads::Mutex::Lock lock(map_lock);
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				image_data = ImageData::create(post);
			} else {
				image_data = iter->second;
			}
		}

		if (!image_data) {
			g_error("Failed to create image_data");
		}

		bool write_error = false;
		auto file = Gio::File::create_for_uri(image_data->get_uri(write_thumb));
		try {
			auto parent = file->get_parent();
			try {
				if (! parent->query_exists())
					parent->make_directory();
			} catch (Gio::Error e) {
			} // Ignore
			
			if ( file->query_exists() ) {
				g_warning("Image already exists on disk: %s. Replacing, but this shouldn't happen.", file->get_uri().c_str());
			} 

			auto ostream = file->replace();
			ostream->splice(istream, 
			                Gio::OUTPUT_STREAM_SPLICE_CLOSE_TARGET |
			                Gio::OUTPUT_STREAM_SPLICE_CLOSE_SOURCE);
		} catch (Gio::Error e) {
			std::cerr << "Error: Unable to save image ("
			          << file->get_uri() << ") to disk: " 
			          << e.what() << std::endl;
			write_error = true;
		}

		if (!write_error) {
			Glib::Threads::Mutex::Lock lock(map_lock);

			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				if (write_thumb)
					image_data->have_thumbnail = true;
				else
					image_data->have_image = true;
				images.insert({post->get_hash(), image_data});
			} else {
				if (write_thumb)
					iter->second->have_thumbnail = true;
				else
					iter->second->have_image = true;
				iter->second->update(post);
			}
		}
	}

	void ImageCache::read_thumb(const Glib::RefPtr<Post> &post,
	                            std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		std::shared_ptr<ImageData> image_data;
		bool is_thumb = true;

		if (post) {
			Glib::Threads::Mutex::Lock lock(map_lock);
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				g_error("Read thumb called when we don't have the thumbnail");
			} else {
				if ( iter->second->have_thumbnail == false ) {
					g_error("Read thumb called when we don't have the thumbnail");
				} else {
					image_data = iter->second;
					if (G_LIKELY(image_data)) {
						image_data->update(post);
					} else {
						g_error("An invalid image_data is in std::map images");
					}
				}
			}
		}

		read(image_data, callback, is_thumb);
	}

	void ImageCache::read_image(const Glib::RefPtr<Post> &post,
	                            std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		std::shared_ptr<ImageData> image_data;
		bool is_thumb = false;
		{
			Glib::Threads::Mutex::Lock lock(map_lock);
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				g_error("Read thumb called when we don't have the thumbnail");
			} else {
				if (iter->second->have_image == false) {
					g_error("Read thumb called when we don't have the thumbnail");
				} else {
					image_data = iter->second;
					if (G_LIKELY(image_data)) {
						image_data->update(post);
					} else {
						g_error("An invalid image_data is in std::map images");
					}
				}
			}
		}

		read(image_data, callback, is_thumb);
	}

	void ImageCache::read(const std::shared_ptr<ImageData> &image_data,
	                      std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback,
	                      bool read_thumb) {
		auto read_error = false;
		auto file = Gio::File::create_for_uri(image_data->get_uri(read_thumb));
		auto loader = Gdk::PixbufLoader::create();

		if (image_data) {
			try {
				if ( !file->query_exists() ) {
					g_warning("Cache believes it has %s on disk, but it doesn't exist.",
					          file->get_uri().c_str());
					read_error = true;
				} else {
					auto istream = file->read();
					auto info = istream->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE);
					const gsize fsize = info->get_size();
					guint8 *buffer = g_new0(guint8, fsize);
					gsize read_bytes = 0;
					istream->read_all(buffer, fsize, read_bytes);
					if (read_bytes != fsize) {
						std::cerr << "Warning: While reading "
						          << file->get_uri() << " from disk, expected "
						          << fsize << " bytes, but got " << read_bytes
						          << std::endl;
						read_error = true;
					} else {
						loader->write(buffer, read_bytes);
					}
					istream->close();
					// Close the loader below.
					g_free(buffer);
				}
			} catch (Gio::Error e) {
				std::cerr << "Error reading image " << file->get_uri()
				          << " from disk: " << e.what() << std::endl;
				read_error = true;
			} catch (Gdk::PixbufError e) {
				std::cerr << "Error creating image from on-disk cache ("
				          << file->get_uri() << ") : " << e.what()
				          << std::endl;
				read_error = true;
			} catch (Glib::FileError e) {
				std::cerr << "Error creating image from on-disk cache ("
				          << file->get_uri() << ") : " << e.what()
				          << std::endl;
				read_error = true;
			}
		} else {
			read_error = true;
		}

		try {
			loader->close();
		} catch (Gdk::PixbufError e) {
			if (!read_error) {
				std::cerr << "Error closing PixbufLoader in ImageCache: " 
				          << e.what() << std::endl;
				read_error = true;
			}
		} catch (Glib::FileError e) {
			if (!read_error) {
				std::cerr << "Error closing PixbufLoader in ImageCache: " 
				          << e.what() << std::endl;
				read_error = true;
			}
		}

		if (read_error)
			loader.reset(); // Deletes the loader

		callback(loader);
	}

	void ImageCache::write_thumb_async(const Glib::RefPtr<Post> &post,
	                                   Glib::RefPtr<Gio::MemoryInputStream> &istream) {
		if (G_UNLIKELY(!post))
			g_error("Invalid post handed to write_thumb_async");

		{
			Glib::Threads::Mutex::Lock lock(thumb_write_queue_lock);
			thumb_write_queue.push_back({post, istream});
		}

		write_queue_w.send();
	}

	void ImageCache::write_image_async(const Glib::RefPtr<Post> &post,
	                                   Glib::RefPtr<Gio::MemoryInputStream> &istream) {
		if (G_UNLIKELY(!post))
			g_error("Invalid post handed to write_image_async");

		{
			Glib::Threads::Mutex::Lock lock(image_write_queue_lock);
			image_write_queue.push_back({post, istream});
		}

		write_queue_w.send();
	}

	void ImageCache::get_thumb_async(const Glib::RefPtr<Post> &post,
	                                 std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		if (G_UNLIKELY(!post))
			g_error("Invalid post handed to get_thumb_async");

		{
			Glib::Threads::Mutex::Lock lock(thumb_read_queue_lock);
			thumb_read_queue.push_back({post, callback});
		}

		read_queue_w.send();
	}

	void ImageCache::get_image_async(const Glib::RefPtr<Post> &post,
	                                 std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		if (G_UNLIKELY(!post))
			g_error("Invalid post handed to get_thumb_async");

		{
			Glib::Threads::Mutex::Lock lock(image_read_queue_lock);
			image_read_queue.push_back({post, callback});
		}

		read_queue_w.send();
	}


	void ImageCache::on_read_queue_w(ev::async &, int) {
		std::vector< std::pair< Glib::RefPtr<Post>,
		                        std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> > > work_list;
		{
			Glib::Threads::Mutex::Lock lock(thumb_read_queue_lock);
			work_list.reserve(thumb_read_queue.size());
			std::copy(thumb_read_queue.begin(),
			          thumb_read_queue.end(),
			          std::back_inserter(work_list));
			thumb_read_queue.clear();
		}

		for (auto pair : work_list) {
			Glib::RefPtr<Post> post = pair.first;
			auto callback = pair.second;
			read_thumb(post, callback);
		}

		work_list.clear();
		
		{
			Glib::Threads::Mutex::Lock lock(image_read_queue_lock);
			work_list.reserve(image_read_queue.size());
			std::copy(image_read_queue.begin(),
			          image_read_queue.end(),
			          back_inserter(work_list));
			image_read_queue.clear();
		}

		for (auto pair : work_list) {
			Glib::RefPtr<Post> post = pair.first;
			auto callback = pair.second;
			read_image(post, callback);
		}

		work_list.clear();
		
		// We may have updated the metadata :)
		flush_w.send();
	}

	void ImageCache::on_write_queue_w(ev::async &, int) {
		std::vector <  std::pair< Glib::RefPtr<Post>,
		                          Glib::RefPtr< Gio::MemoryInputStream > > > work_list;

		bool is_thumb = true;
		{
			Glib::Threads::Mutex::Lock lock(thumb_write_queue_lock);
			work_list.reserve(thumb_write_queue.size());
			std::copy(thumb_write_queue.begin(),
			          thumb_write_queue.end(),
			          std::back_inserter(work_list));
			thumb_write_queue.clear();
		}

		for ( auto pair : work_list ) {
			Glib::RefPtr<Post> post = pair.first;
			Glib::RefPtr< Gio::MemoryInputStream > stream = pair.second;
			write(post, stream, is_thumb);
		}

		work_list.clear();

		is_thumb = false;
		{
			Glib::Threads::Mutex::Lock lock(image_write_queue_lock);
			work_list.reserve(image_write_queue.size());
			std::copy(image_write_queue.begin(),
			          image_write_queue.end(),
			          std::back_inserter(work_list));
			image_write_queue.clear();
		}

		for ( auto pair : work_list ) {
			Glib::RefPtr<Post> post = pair.first;
			Glib::RefPtr< Gio::MemoryInputStream > stream = pair.second;
			write(post, stream, is_thumb);
		}

		work_list.clear();
		flush_w.send();
	}

	static std::string get_cache_file_path() {
		const std::vector<std::string> path_parts = { Glib::get_user_data_dir(),
		                                              "horizon",
		                                              CACHE_FILENAME };
		const std::string path = Glib::build_filename( path_parts );
		return path;
	}

	void ImageCache::on_flush_w(ev::async &, int) {
		flush();
	}

	void ImageCache::flush() {
		std::vector< std::pair<std::string, std::shared_ptr<ImageData> > > work_list;
		{
			Glib::Threads::Mutex::Lock lock(map_lock);
			work_list.reserve(images.size());
			std::copy(images.begin(),
			          images.end(),
			          std::back_inserter(work_list));
		}

		std::vector< Glib::VariantContainerBase > variants;
		variants.reserve(work_list.size());

		std::transform(work_list.begin(),
		               work_list.end(),
		               std::back_inserter(variants),
		               [](std::pair<std::string, std::shared_ptr<ImageData> > pair) {
			               std::shared_ptr<ImageData> data(pair.second);
			               if (data) {
				               Glib::VariantContainerBase v = data->get_variant();
				               return v;
			               } else {
				               throw std::range_error("Invalid pointer in work_list");
			               }
		               });

		std::vector<GVariant*> cvariants;
		cvariants.reserve(variants.size());
		
		std::transform(variants.begin(),
		               variants.end(),
		               std::back_inserter(cvariants),
		               [](Glib::VariantContainerBase variant) {
			               return variant.gobj_copy();
		               });

		GVariantType *vt = g_variant_type_new(CACHE_VERSION_1_TYPE.c_str());
		GVariant *varray = g_variant_new_array(vt,
		                                       cvariants.data(),
		                                       cvariants.size());
		
		try {
			auto file = Gio::File::create_for_path(get_cache_file_path());
			auto ostream = file->replace();
			gsize written = 0;
			ostream->write_all(&CACHE_FILE_VERSION, sizeof(guint32), written);
			if ( written < sizeof(guint32) ) {
				g_error("Unable to write version information");
			}
			ostream->write_all(g_variant_get_data(varray),
			                   g_variant_get_size(varray),
			                   written);
			ostream->close();
		} catch (Gio::Error e) {
			g_error("Failed to write ImageCache file: %s", e.what().c_str());
		}

		g_variant_unref(varray);
		g_variant_type_free(vt);

		std::for_each(cvariants.begin(),
		              cvariants.end(),
		              [](GVariant* cvariant) {
			              g_variant_unref(cvariant);
		              });
	}

	static void buffer_destroy_notify(gpointer data) {
		g_free(data);
	}

	void ImageCache::read_from_disk() {
		Glib::Threads::Mutex::Lock lock(map_lock);
		auto file = Gio::File::create_for_path(get_cache_file_path());

		if (! file->query_exists() ) {
			auto parent = file->get_parent();
			if (! parent->query_exists() ) {
				try {
					bool res = parent->make_directory_with_parents();
					if (!res) {
						g_error ("Unable to create directory %s", parent->get_uri().c_str());
					} else {
						std::cerr << "Created directory " << parent->get_uri() << std::endl;
					}
				} catch ( Gio::Error e ) {
				} // Ignore. this happens if ~/.local/share/
				  // exists. Other failures are caught above.
				try {
					parent->get_child("thumbs")->make_directory();
					parent->get_child("images")->make_directory();
				} catch ( Gio::Error e) {
				}
			}

			try {
				auto ostream = file->create_file();
				if (ostream) {
					std::cerr << "Created file " << file->get_uri() << std::endl;
					ostream->close();
				}
			} catch (Gio::Error e) {
				g_error("Failed to create ImageCache file: %s", e.what().c_str());
			}
		}

		goffset fsize = 0;
		try {
			auto fileinfo = file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE);
			fsize = fileinfo->get_size();
		} catch (Gio::Error e) {
			g_error("Failed to read info about our ImageCache file: %s", e.what().c_str());
		}


		if (fsize > 0) {
			try {
				auto istream = file->read();
				guint32 version = 0;
				gsize read_bytes = 0;
				if (istream->read_all(&version, sizeof(guint32), read_bytes)) {
					if (read_bytes < sizeof(guint32))
						g_error("Failed to read version header of ImageCache.");
					fsize -= read_bytes;
					if (fsize > 0) {
						void *buffer = g_malloc(fsize);
						if (istream->read_all(buffer,
						                      static_cast<gsize>(fsize),
						                      read_bytes)) {
							GVariantType *gvt = g_variant_type_new(CACHE_VERSION_1_ARRAYTYPE.c_str());
							GVariant *v = g_variant_new_from_data(gvt, buffer, 
							                                      fsize, FALSE,
							                                      buffer_destroy_notify,
							                                      buffer);
							if ( ! g_variant_is_normal_form(v) ) {
								g_error ( "ImageCache is corrupted. You might want to delete everything under %s",
								          file->get_parent()->get_uri().c_str() );
							}
							gsize elements = g_variant_n_children( v );

							for ( gsize i = 0; i < elements; i++ ) {
								GVariant *child = g_variant_get_child_value(v, i);
								auto cpp_child = Glib::wrap(child, true);
								auto typed_cpp_child = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(cpp_child);

								std::shared_ptr<ImageData> cp = ImageData::create(version, typed_cpp_child);
								if (cp)
									images.insert({cp->md5, cp});

								g_variant_unref(child);
							}
							g_variant_unref(v);
							g_variant_type_free(gvt);

						} else {
							g_error("Failed to read the ImageCache serialized data");
						}
					}
				}
				istream->close();
			} catch (Gio::Error e) {
				g_error("Failure reading ImageCache file: %s", e.what().c_str());
			}
		}
	}

	void ImageCache::loop() {
		read_from_disk();

		ev_loop.run();
	}

	void ImageCache::on_kill_loop_w(ev::async &, int) {
		write_queue_w.stop();
		read_queue_w.stop();
		flush_w.stop();
		kill_loop_w.stop();
	}

	ImageCache::~ImageCache() {
		flush_w.send();
		kill_loop_w.send();
		ev_thread->join();
	}

	ImageCache::ImageCache() :
		ev_loop(ev::AUTO),
		kill_loop_w(ev_loop),
		write_queue_w(ev_loop),
		read_queue_w(ev_loop),
		flush_w(ev_loop)
	{
		write_queue_w.set<ImageCache, &ImageCache::on_write_queue_w>(this);
		read_queue_w.set<ImageCache, &ImageCache::on_read_queue_w>(this);
		kill_loop_w.set<ImageCache, &ImageCache::on_kill_loop_w>(this);
		flush_w.set<ImageCache, &ImageCache::on_flush_w>(this);
		write_queue_w.start();
		read_queue_w.start();
		kill_loop_w.start();
		flush_w.start();

		ev_thread = Glib::Threads::Thread::create( sigc::mem_fun(*this, &ImageCache::loop) );
		if (!ev_thread)
			g_error ("Couldn't spin up ImageCache thread.");
	}

}
