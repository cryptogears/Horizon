#include <array>
#include <iostream>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <glibmm/uriutils.h>
#include "image_cache.hpp"
#include "utils.hpp"

namespace Horizon {

	struct SArrayGFreer {
		void operator()(const gchar** s) {
			g_free(s);
		}
	};

	ImageData::ImageData(const guint32 version, const std::unique_ptr<GVariant, VariantUnrefer> cvariant) {
		const gsize cvariant_children = g_variant_n_children(cvariant.get());
		if ( version == 1 ) {
			if (cvariant_children != 12) {
				g_error("Invalid number of elements in version 1 data: %" G_GSIZE_FORMAT " elements.",
				        cvariant_children);
			} else {
				typedef std::unique_ptr<GVariant, VariantUnrefer> v_ptr;
				typedef std::unique_ptr<const gchar*, SArrayGFreer> sarray_ptr;
				const gchar *string;
				g_variant_get_child(cvariant.get(), 0, "t", &size);
				g_variant_get_child(cvariant.get(), 1, "^&ay", &string);
				md5 = string;
				g_variant_get_child(cvariant.get(), 2, "^&ay", &string);
				ext = string;
				const v_ptr vboards   (g_variant_get_child_value(cvariant.get(), 3));
				const v_ptr vtags     (g_variant_get_child_value(cvariant.get(), 4));
				const v_ptr vfilenames(g_variant_get_child_value(cvariant.get(), 5));
				const v_ptr vposters  (g_variant_get_child_value(cvariant.get(), 6));
				const v_ptr vdates    (g_variant_get_child_value(cvariant.get(), 7));
				g_variant_get_child(cvariant.get(), 8, "q",&num_spoiler);
				g_variant_get_child(cvariant.get(), 9, "q",&num_deleted);
				g_variant_get_child(cvariant.get(), 10, "b", &have_thumbnail);
				g_variant_get_child(cvariant.get(), 11, "b", &have_image);

				if (vboards) {
					const gsize arraysize = g_variant_n_children(vboards.get());
					if (arraysize > 0) {
						const sarray_ptr sarray(g_variant_get_strv(vboards.get(),
						                                           nullptr));
						for ( gsize i = 0; i < arraysize; ++i ) {
							boards.insert(sarray.get()[i]);
						}
					}
				}

				if (vtags) {
					const gsize arraysize = g_variant_n_children(vtags.get());
					if (arraysize > 0) {
						const sarray_ptr sarray(g_variant_get_strv(vtags.get(),
						                                           nullptr));
						for ( gsize i = 0; i < arraysize; i++ ) {
							tags.insert(sarray.get()[i]);
						}
					}
				}
				
				if (vfilenames) {
					const gsize arraysize = g_variant_n_children(vfilenames.get());
					if (arraysize > 0) {
						const sarray_ptr sarray(g_variant_get_bytestring_array(vfilenames.get(),
						                                                       nullptr));
						for ( gsize i = 0; i < arraysize; i++ ) {
							original_filenames.insert(sarray.get()[i]);
						}
					}
				}

				if (vposters) {
					const gsize arraysize = g_variant_n_children(vposters.get());
					if (arraysize > 0) {
						const sarray_ptr sarray(g_variant_get_bytestring_array(vposters.get(),
						                                                 nullptr));
						for ( gsize i = 0; i < arraysize; i++ ) {
							poster_names.insert(sarray.get()[i]);
						}
					}
				}

				if (vdates) {
					gsize arraysize = 0;
					const gint64 *sizes_array = static_cast<const gint64*>(g_variant_get_fixed_array(vdates.get(), &arraysize, sizeof(gint64)));
					for ( gsize i = 0; i < arraysize; i++ ) {
						posted_unix_dates.insert(sizes_array[i]);
					}
				}
			}
		} else {
			g_error("Invalid ImageCache version %" G_GUINT32_FORMAT, version);
		}
	}

	ImageData::ImageData(const guint32 version, const Glib::VariantContainerBase& variant) {
		/*
		  Glib::Variant<>::create has a memory leak in Gtk < 3.8
		 */
		gsize num = variant.get_n_children();
		if ( version == 1 ) {
			if (num != 12) {
				g_error("Invalid number of elements in version 1 data: %" G_GSIZE_FORMAT " elements.", num);
			}
			Glib::Variant< guint64 >                      vsize;
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
			std::vector<Glib::ustring> vec_boards = vboards    .get();
			std::vector<Glib::ustring> vec_tags   = vtags      .get();
			std::vector<std::string>   filenames  = vfilenames .get();
			std::vector<std::string>   posters    = vposters   .get();
			std::vector<gint64>        dates      = vdates     .get();
			size                                  = vsize      .get();
			md5                                   = vmd5       .get();
			ext                                   = vext       .get();
			num_spoiler                           = vspoilers  .get();
			num_deleted                           = vdeleted   .get();
			have_thumbnail                        = vhave_thumb.get();
			have_image                            = vhave_image.get();
			std::copy(vec_boards.rbegin(), vec_boards.rend(), std::inserter(boards,             boards            .begin()));
			std::copy(vec_tags  .rbegin(), vec_tags  .rend(), std::inserter(tags,               tags              .begin()));
			std::copy(filenames .rbegin(), filenames .rend(), std::inserter(original_filenames, original_filenames.begin()));
			std::copy(posters   .rbegin(), posters   .rend(), std::inserter(poster_names,       poster_names      .begin()));
			std::copy(dates     .rbegin(), dates     .rend(), std::inserter(posted_unix_dates,  posted_unix_dates .begin()));
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

	void ImageData::merge(const std::unique_ptr<ImageData> &in) {
		if ( md5.find(in->md5) == md5.npos ) {
			g_warning("ImageData::merge() called with wrong hash. These shouldn't be merged!");
			return;
		}
		
		boards.insert            (in->boards.begin(),
		                          in->boards.end());
		tags.insert              (in->tags.begin(),
		                          in->tags.end());
		original_filenames.insert(in->original_filenames.begin(),
		                          in->original_filenames.end());
		poster_names.insert      (in->poster_names.begin(),
		                          in->poster_names.end());
		posted_unix_dates.insert (in->posted_unix_dates.begin(),
		                          in->posted_unix_dates.end());
		num_spoiler   += in->num_spoiler;
		num_deleted   += in->num_deleted;
		have_thumbnail = have_thumbnail | in->have_thumbnail;
		have_image     = have_image | in->have_image;
	}

	/*
	  This will lean in Gtk < 3.8
	 */
	Glib::VariantContainerBase ImageData::get_variant() const {
		auto vsize = Glib::Variant<guint64>::create(size);
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

	GVariant* ImageData::get_cvariant() const {
		GVariantBuilder boards_builder, tags_builder, filenames_builder, posters_builder, dates_builder;
		const std::unique_ptr<GVariantType, VariantTypeDeleter> dates_type(g_variant_type_new_array(G_VARIANT_TYPE_INT64));
		g_variant_builder_init(&boards_builder,
		                       G_VARIANT_TYPE_STRING_ARRAY);
		g_variant_builder_init(&tags_builder,
		                       G_VARIANT_TYPE_STRING_ARRAY);
		g_variant_builder_init(&filenames_builder,
		                       G_VARIANT_TYPE_BYTESTRING_ARRAY);
		g_variant_builder_init(&posters_builder,
		                       G_VARIANT_TYPE_BYTESTRING_ARRAY);
		g_variant_builder_init(&dates_builder,
		                       dates_type.get());

		for ( auto board : boards ) {
			g_variant_builder_add_value(&boards_builder,
			                            g_variant_new_string(board.c_str()));
		}

		for ( auto tag : tags ) {
			g_variant_builder_add_value(&tags_builder,
			                            g_variant_new_string(tag.c_str()));
		}

		for ( auto filename : original_filenames ) {
			g_variant_builder_add_value(&filenames_builder,
			                            g_variant_new_bytestring(filename.c_str()));
		}

		for ( auto poster : poster_names ) {
			g_variant_builder_add_value(&posters_builder,
			                            g_variant_new_bytestring(poster.c_str()));
		}

		for ( auto date : posted_unix_dates ) {
			g_variant_builder_add_value(&dates_builder,
			                            g_variant_new_int64(date));
		}
		
		std::array<GVariant*, 12> varray{ {g_variant_new_uint64(size),
					g_variant_new_bytestring(md5.c_str()),
					g_variant_new_bytestring(ext.c_str()),
					g_variant_builder_end(&boards_builder),
					g_variant_builder_end(&tags_builder),
					g_variant_builder_end(&filenames_builder),
					g_variant_builder_end(&posters_builder),
					g_variant_builder_end(&dates_builder),
					g_variant_new_uint16(num_spoiler),
					g_variant_new_uint16(num_deleted),
					g_variant_new_boolean(have_thumbnail),
					g_variant_new_boolean(have_image)} };
					
		GVariant *cvariant = g_variant_new_tuple(varray.data(), varray.size());

		return cvariant;
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
	                       const bool write_thumb) {
		if (write_thumb && has_thumb(post)) {
			g_warning("write called for thumbnail when we already have a thumbnail");
			return;
		} else if ((!write_thumb) && has_image(post)) {
			g_warning("write called for image when we already have an image");
			return;
		}

		Glib::RefPtr<Gio::File> file;
		bool write_error = false;
		{
			Glib::Threads::Mutex::Lock lock(map_lock);
			std::map<std::string, std::unique_ptr<ImageData> >::iterator image_data_iter;
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				std::unique_ptr<ImageData> ptr(new ImageData(post));
				auto pair = images.insert(std::make_pair(ptr->md5, std::move(ptr)));
				if (pair.second) {
					image_data_iter = pair.first;
				} else {
					g_error("Failed to insert new image data");
				}
			} else {
				image_data_iter = iter;
			}
			file = Gio::File::create_for_uri(image_data_iter->second->get_uri(write_thumb));
		}

		if (!file) {
			std::cerr << "Error: Failed to build filename for image ";
			if (write_thumb)
				std::cerr << post->get_thumb_url() << std::endl;
			else
				std::cerr << post->get_image_url() << std::endl;
			return;
		}

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
			if (iter != images.end()) {
				if (write_thumb)
					iter->second->have_thumbnail = true;
				else
					iter->second->have_image = true;
			}
		}
	}

	void ImageCache::read_thumb(const Glib::RefPtr<Post> &post,
	                            std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		Glib::RefPtr<Gio::File> file;
		constexpr bool is_thumb = true;

		if (post) {
			Glib::Threads::Mutex::Lock lock(map_lock);
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				g_error("Read thumb called when we don't have the thumbnail");
			} else {
				if ( iter->second->have_thumbnail == false ) {
					g_error("Read thumb called when we don't have the thumbnail");
				} else {
					if (G_LIKELY(iter->second)) {
						iter->second->update(post);
						file = Gio::File::create_for_uri(iter->second->get_uri(is_thumb));
					} else {
						g_error("An invalid image_data is in std::map images");
					}
				}
			}
		}

		if (file)
			read(file, callback);
	}

	void ImageCache::read_image(const Glib::RefPtr<Post> &post,
	                            std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		Glib::RefPtr<Gio::File> file;
		constexpr bool is_thumb = false;
		{
			Glib::Threads::Mutex::Lock lock(map_lock);
			auto iter = images.find(post->get_hash());
			if ( iter == images.end() ) {
				g_error("Read thumb called when we don't have the thumbnail");
			} else {
				if (iter->second->have_image == false) {
					g_error("Read thumb called when we don't have the thumbnail");
				} else {
					if (G_LIKELY(iter->second)) {
						iter->second->update(post);
						file = Gio::File::create_for_uri(iter->second->get_uri(is_thumb));
					} else {
						g_error("An invalid image_data is in std::map images");
					}
				}
			}
		}
		
		if (file)
			read(file, callback);
	}
	
	void ImageCache::read(const Glib::RefPtr<Gio::File>& file,
	                      std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> callback) {
		auto read_error = false;
		auto loader = Gdk::PixbufLoader::create();
		
		if (file) {
			try {
				if ( !file->query_exists() ) {
					g_warning("Cache believes it has %s on disk, but it doesn't exist.",
					          file->get_uri().c_str());
					read_error = true;
				} else {
					auto istream = file->read();
					if (!istream->can_seek()) {
						g_error("Filesystem holding cache does not allow seeks.");
					}
					istream->seek(0, Glib::SEEK_TYPE_END);
					const gsize fsize = istream->tell();
					istream->seek(0, Glib::SEEK_TYPE_SET);
					std::unique_ptr<guint8[]> buffer(new guint8[fsize]);
					gsize read_bytes = 0;
					istream->read_all(buffer.get(), fsize, read_bytes);
					if (read_bytes != fsize) {
						std::cerr << "Warning: While reading "
						          << file->get_uri() << " from disk, expected "
						          << fsize << " bytes, but got " << read_bytes
						          << std::endl;
						read_error = true;
					} else {
						// BUG LEAK: gdk_pixbuf_loader_write() leaks
						// randomly (but rarely)
						loader->write(buffer.get(), read_bytes);
					}
					istream->close();
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
	}

	void ImageCache::on_flush_w(ev::async &, int) {
		flush();
	}

	void ImageCache::clean_invalid() {
		Glib::Threads::Mutex::Lock lock(map_lock);
		auto iter = images.begin();
		while (iter != images.end()) {
			iter = std::find_if(images.begin(),
			                    images.end(),
			                    [](const std::pair<const std::string&, const std::unique_ptr<ImageData>& >& pair) {
				                    return !(pair.second->have_image || pair.second->have_thumbnail);});
			if ( iter != images.end() ) {
				std::cerr << "Info: Deleting invalid image cache entry..." << std::endl;
				images.erase(iter);
			}
		}				              
	}

	void ImageCache::flush() {
		clean_invalid();
		std::cout << "Info: Flushing..." << std::flush;
		std::vector<GVariant*> cvariants;
		{
			std::cout << " (locking)" << std::flush;
			Glib::Threads::Mutex::Lock lock(map_lock);
			cvariants.reserve(images.size());

			std::transform(images.begin(),
			               images.end(),
			               std::back_inserter(cvariants),
			               // pair.second->get_cvariant()
			               std::bind(std::mem_fn(&ImageData::get_cvariant),
			                         std::bind(std::mem_fn(&std::pair<const std::string,
			                                               std::unique_ptr<ImageData> >::second),
			                                   std::placeholders::_1))
			               );
		}
		std::cout << " (unlocked)" << std::flush;

		if (cvariants.size() > 0) {
			// do not free vt
			const GVariantType * const vt = g_variant_get_type(cvariants.front());

			typedef std::unique_ptr<GVariant, VariantUnrefer> v_ptr;

			const v_ptr varray(g_variant_ref_sink(
			                   g_variant_new_array(vt,
			                                       cvariants.data(),
			                                       cvariants.size())));
			const gsize data_size = g_variant_get_size(varray.get());
			const std::unique_ptr<guint8[]> data(new guint8[data_size]);
			g_variant_store(varray.get(), data.get());
		
			try {
				const std::string etag;
				constexpr bool make_backup = true;
				constexpr Gio::FileCreateFlags fcflags = Gio::FILE_CREATE_NONE;
				auto ostream = cache_file->replace(etag, make_backup, fcflags);
				gsize written = 0;
				ostream->write_all(&CACHE_FILE_VERSION, sizeof(guint32), written);
				if ( written < sizeof(guint32) ) {
					g_error("Unable to write version information");
				}
				ostream->write_all(data.get(),
				                   data_size,
				                   written);
				if ( written < data_size ) {
					g_error("Unable to write complete ImageCache: %" G_GSIZE_FORMAT
					        " of %" G_GSIZE_FORMAT ".", written, data_size);
				}
				ostream->close();
			} catch (Gio::Error e) {
				g_error("Failed to write ImageCache file: %s", e.what().c_str());
			}
		}

		std::cout << " done." << std::endl;
	}

	/*
	 * Blocking operation
	 */
	void ImageCache::merge_file(const Glib::RefPtr<Gio::File>& file) {
		read_from_disk(file, false);
	}

	void ImageCache::read_from_disk(const Glib::RefPtr<Gio::File>& file,
	                                const bool make_dirs) {
		if (!file) {
			std::cerr << "Error: Invalid file pointer passed to read_from_disk"
			          << std::endl;
			return;
		}

		Glib::Threads::Mutex::Lock lock(map_lock);
		const bool file_exists = file->query_exists();
		if (G_UNLIKELY( !file_exists && make_dirs )) {
			auto parent = file->get_parent();
			if (! parent->query_exists() ) {
				try {
					bool res = parent->make_directory_with_parents();
					if (!res) {
						g_error ("Unable to create directory %s", parent->get_uri().c_str());
					} else {
						std::cerr << "Created directory " << parent->get_uri() << std::endl;
					}
					parent->get_child("thumbs")->make_directory();
					parent->get_child("images")->make_directory();
				} catch ( Gio::Error e) {
				// Ignore. this happens if ~/.local/share/
				// exists. Other failures are caught above.
				}
			}
		}

		try {
			auto istream = file->read();
			if (!istream->can_seek()) {
				g_error("Filesystem doesn't support seeking");
			} 
			istream->seek(0, Glib::SEEK_TYPE_END);
			goffset fsize = istream->tell();
			istream->seek(0, Glib::SEEK_TYPE_SET);
			if (G_UNLIKELY( fsize <= 0 )) {
				return;
			}
			guint32 version = 0;
			gsize read_bytes = 0;
			if (G_UNLIKELY( !istream->read_all(&version,
			                                   sizeof(guint32),
			                                   read_bytes) )) {
				std::cerr << "Error: Couldn't read image cache versioning."
				          << std::endl;
				return;
			}

			if (G_UNLIKELY( read_bytes < sizeof(guint32))) {
				std::cerr << "Error: Couldn't read image cache versioning."
				          << std::endl;
				return;
			}

			fsize -= read_bytes;
			if (G_UNLIKELY( fsize <= 0 )) {
				return;
			}
			std::unique_ptr<gchar> buffer(new gchar[fsize]);
			if (G_UNLIKELY( !istream->read_all(buffer.get(),
			                                   static_cast<gsize>(fsize),
			                                   read_bytes) )) {
				std::cerr << "Error: Couldn't read image cache"
				          << std::endl;
				return;
			}

			typedef std::unique_ptr<GVariantType, VariantTypeDeleter> vtype_ptr;
			typedef std::unique_ptr<GVariant, VariantUnrefer> v_ptr;
			typedef std::unique_ptr<ImageData> idata_ptr;

			const gchar* vtype = nullptr;
			if (G_LIKELY( version == 1 )) {
				vtype = CACHE_VERSION_1_ARRAYTYPE;
			} else {
				std::cerr << "Error: Unsupported image cache version "
				          << version << std::endl;
				return;
			}

			const vtype_ptr gvt(g_variant_type_new(vtype));
			const v_ptr v_untrusted(g_variant_ref_sink(
                                    g_variant_new_from_data(gvt.get(),
                                                            buffer.get(), 
                                                            fsize,
                                                            FALSE,
                                                            nullptr,
                                                            nullptr)));
			const v_ptr v(g_variant_get_normal_form(v_untrusted.get()));
			if (G_UNLIKELY( ! g_variant_is_normal_form(v.get()) )) {
				std::cerr << "Error: ImageCache " << file->get_parse_name()
				          << " may be corrupted." << std::endl;
				return;
			}

			const gsize elements = g_variant_n_children( v.get() );
							
			std::cout << "Info: " << file->get_parse_name()
			          << " has information on " << elements
			          << " images." << std::endl;
							
			for ( gsize i = 0; i < elements; ++i ) {
				v_ptr child(g_variant_get_child_value(v.get(), i));
				idata_ptr cp(new ImageData(version, std::move(child)));
				if (G_LIKELY( cp )) {
					auto iter = images.find(cp->md5);
					if (G_LIKELY( iter == images.end() )) {
						images.insert(std::make_pair(cp->md5,
						                             std::move(cp)));
					} else {
						iter->second->merge(std::move(cp));
					}
				}
			}

			istream->close();
		} catch (Gio::Error e) {
			std::cerr << "Error: While reading image cache " 
			          << file->get_parse_name() << ": " 
			          << e.what();
		}
	}

	void ImageCache::loop() {
		timer_w.set(0., 60. * 5.);
		timer_w.again();
		read_from_disk(cache_file, true);

		ev_loop.run();
	}

	void ImageCache::on_timer_w(ev::timer &w, int) {
		flush_w.send();
		w.again();
	}

	void ImageCache::on_kill_loop_w(ev::async &, int) {
		timer_w.stop();
		write_queue_w.stop();
		read_queue_w.stop();
		flush_w.stop();
		kill_loop_w.stop();
	}

	ImageCache::~ImageCache() {
		flush();
		kill_loop_w.send();
		ev_thread->join();
	}

	ImageCache::ImageCache(const Glib::RefPtr<Gio::File>& file) :
		cache_file(file),
		ev_thread(nullptr),
		ev_loop(ev::AUTO | ev::POLL),
		kill_loop_w(ev_loop),
		write_queue_w(ev_loop),
		read_queue_w(ev_loop),
		flush_w(ev_loop),
		timer_w(ev_loop)
	{
		write_queue_w.set<ImageCache, &ImageCache::on_write_queue_w>(this);
		read_queue_w. set<ImageCache, &ImageCache::on_read_queue_w> (this);
		kill_loop_w.  set<ImageCache, &ImageCache::on_kill_loop_w>  (this);
		flush_w.      set<ImageCache, &ImageCache::on_flush_w>      (this);
		timer_w.      set<ImageCache, &ImageCache::on_timer_w>      (this);
		write_queue_w.start();
		read_queue_w.start();
		kill_loop_w.start();
		flush_w.start();

		const sigc::slot<void> slot = sigc::mem_fun(*this, &ImageCache::loop);
		int trycount = 0;

		while ( ev_thread == nullptr && trycount++ < 10 ) {
			try {
				ev_thread = Horizon::create_named_thread("ImageCache",
				                                         slot);
			} catch ( Glib::Threads::ThreadError e) {
				if (e.code() != Glib::Threads::ThreadError::AGAIN) {
					g_error("Couldn't create ImageFetcher thread: %s",
					        e.what().c_str());
				}
			}
		}
		

		if (!ev_thread)
			g_error ("Couldn't spin up ImageCache thread.");
	}
}
