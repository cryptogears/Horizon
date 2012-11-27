
#include "horizon_image.hpp"
#include <iostream>
#include <glibmm/main.h>

namespace Horizon {

	Image::Image(const Glib::RefPtr<Post> &post_,
	             std::shared_ptr<ImageFetcher> image_fetcher,
	             sigc::slot<void, const ImageState&> state_change_callback) :
		Gtk::Container(),
		post(post_),
		event_box(Gtk::manage(new Gtk::EventBox())),
		image(Gtk::manage(new Gtk::Image())),
		image_state(NONE),
		is_changing_state(false),
		scaled_width(-1),
		scaled_height(-1),
		is_scaled(false),
		am_fetching_thumb(false),
		am_fetching_image(false),
		canceller(std::make_shared<Canceller>()),
		ifetcher(std::move(image_fetcher))
	{
		set_has_window(false);
		set_redraw_on_allocate(false);
		set_name("imagewindow");
		event_box->set_events(Gdk::BUTTON_PRESS_MASK);
		event_box->set_visible_window(false);
		event_box->signal_button_press_event().connect( sigc::mem_fun(*this, &Image::on_image_click) );
		image->set_name("image");
		image->set_halign(Gtk::ALIGN_START);
		image->set_valign(Gtk::ALIGN_START);
		event_box->set_parent(*this);
		image->set_parent(*this);

		state_changed_dispatcher.connect(sigc::mem_fun(*this,
		                                               &Image::run_state_changed_callbacks));
		signal_state_changed.connect(state_change_callback);

		if (post->has_image()) {
			std::string ext = post->get_image_ext();
			if (ext.compare(".gif") == 0) {
				fetch_image();
			} else {
				fetch_thumbnail();
			}
		} else {
			g_error("Horizon::Image created for post %" G_GINT64_FORMAT
			        " that has no image.",
			        post->get_id()); 
		}

		image->signal_unmap()    .connect(sigc::mem_fun(*this, &Image::on_image_unmap));
		image->signal_unrealize().connect(sigc::mem_fun(*this, &Image::on_image_unrealize));
		image->signal_draw()     .connect(sigc::mem_fun(*this, &Image::on_image_draw), false);
	}

	Image::~Image() {
		canceller->cancel();
		image->unparent();
		image = nullptr;
		event_box->unparent();
		event_box = nullptr;
	}

	void Image::reset_animation_iter() {
		if ( animation_timeout.connected() )
			animation_timeout.disconnect();
		animation_iter.reset();
	}

	void Image::on_image_unmap() {
		reset_animation_iter();
	}

	void Image::on_image_unrealize() {
		reset_animation_iter();
	}

	bool Image::on_image_draw(const Cairo::RefPtr< Cairo::Context > &) {
		if (unscaled_animation) {
			if (!animation_iter) {
				animation_time.assign_current_time();
				
				animation_iter = Glib::wrap(gdk_pixbuf_animation_get_iter(unscaled_animation->gobj(), &animation_time));
				unscaled_image.reset();
				unscaled_image = animation_iter->get_pixbuf();

				animation_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Image::on_animation_timeout),
				                                                   animation_iter->get_delay_time());
			}
		}

		return false;
	}

	void Image::run_state_changed_callbacks() const {
		signal_state_changed(image_state);
	}

	void Image::fetch_thumbnail() {
		if (G_UNLIKELY(thumbnail_image)) {
			g_warning("fetch_thumbnail called when we already have an thumbnail pixbuf");
			return;
		}

		if (!am_fetching_thumb) {
			am_fetching_thumb = true;
			std::function<void (const Glib::RefPtr<Gdk::PixbufLoader>&)> cb = std::bind(&Image::on_thumb, this, std::placeholders::_1);
			if (auto sp_ifetcher = ifetcher.lock()) {
				sp_ifetcher->download(post, cb, canceller, true, nullptr, nullptr);
			}
		}
	}

	void Image::fetch_image() {
		if ( G_UNLIKELY( unscaled_image || unscaled_animation ) ) {
			g_warning("fetch_image called when we already have an image pixbuf");
			return;
		}

		if ( !am_fetching_image ) {
			am_fetching_image = true;
			auto callback = std::bind(&Image::on_image, this, std::placeholders::_1);
			auto area_prepared = std::bind(&Image::on_area_prepared,
			                               this,
			                               std::placeholders::_1);
			auto area_updated = std::bind(&Image::on_area_updated,
			                              this,
			                              std::placeholders::_1,
			                              std::placeholders::_2,
			                              std::placeholders::_3,
			                              std::placeholders::_4);
			if (auto sp_ifetcher = ifetcher.lock()) {
				sp_ifetcher->download(post, callback, canceller, false, area_prepared, area_updated);
			}
		}
	}

	void Image::on_area_prepared(Glib::RefPtr<Gdk::Pixbuf> pixbuf) {
		unscaled_image = pixbuf;
		
		if (is_changing_state) {
			is_changing_state = false;
			set_expand_state();
		}

		if (image_state == NONE) {
			set_thumb_state();
		}
	}

	void Image::on_area_updated(int x, int y, int width, int height) {
		if (scaled_image && scaled_width > 0 && scaled_height > 0) {
			scaled_width = 0;
			scaled_height = 0;
			queue_resize();
			image->queue_draw();
		} else {
			image->queue_draw_area(x, y, width, height);
		}
	}

	void Image::on_thumb(const Glib::RefPtr<Gdk::PixbufLoader> &loader) {
		if (loader) {
			thumbnail_image.reset();
			thumbnail_image = loader->get_pixbuf();
			if (G_UNLIKELY(!thumbnail_image)) {
				g_error("Received pixbuf thumb is null");
			}
		} else {
			std::cerr << "Warning: Horizon::Image got invalid PixbufLoader for"
			          << " thumbnail" << std::endl;
		}

		if (image_state == NONE)
			set_thumb_state();

		am_fetching_thumb = false;
	}

	bool Image::on_animation_timeout() {
		animation_time.assign_current_time();
		if (animation_iter) {
			if (animation_iter->advance(animation_time)) {
				unscaled_image.reset();
				unscaled_image = animation_iter->get_pixbuf();

				if (is_scaled) {
					set_new_scaled_image(scaled_width, scaled_height);
				} else {
					image->set(unscaled_image);
				}
				image->queue_draw();
			}
		
			animation_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Image::on_animation_timeout),
			                                                   animation_iter->get_delay_time());
		}
			
		return false;
	}

	/* 
	   TODO: have image_fetcher tell us if it was a 404 and handle
	   that image here rather than there.
	 */
	void Image::on_image(const Glib::RefPtr<Gdk::PixbufLoader> &loader) {
		if (loader) {
			unscaled_image.reset();

			if (post->is_gif()) {
				unscaled_animation = loader->get_animation();
				if (unscaled_animation->is_static_image()) {
					unscaled_image = unscaled_animation->get_static_image();
					unscaled_animation.reset();
				} else {
					if (!animation_iter) {
						animation_time.assign_current_time();
						animation_iter = Glib::wrap(gdk_pixbuf_animation_get_iter(unscaled_animation->gobj(), &animation_time));

						animation_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Image::on_animation_timeout),
						                                                   animation_iter->get_delay_time());
					}
					unscaled_image.reset();
					unscaled_image = animation_iter->get_pixbuf();
				}
			} else {
				unscaled_image = loader->get_pixbuf();
			}

			if (G_UNLIKELY(!unscaled_image)) {
				g_error("Received pixbuf image is null");
			}
			
			image->set(unscaled_image);
			image->show();
			show_all();
			image->queue_draw();
			refresh_size_request();

			if (is_changing_state) {
				is_changing_state = false;
				set_expand_state();
			}

			if (image_state == NONE)
				set_thumb_state();
		} else {
			std::cerr << "Warning: Horizon::Image got invalid PixbufLoader for"
			          << " image" << std::endl;
		}

		am_fetching_image = false;
	}

	void Image::set_thumb_state() {
		try {
			if (!image->get_pixbuf()) {
				if (thumbnail_image) {
					image->set(thumbnail_image);
				} else if (unscaled_image) {
					image->set(unscaled_image);
				}
			}
			
			image_state = THUMBNAIL;
			state_changed_dispatcher();
			refresh_size_request();
			show_all();
		} catch (Glib::Error e) {
			g_warning("Error creating image from pixmap: %s", e.what().c_str());
		}

	}

	void Image::set_expand_state() {
		image_state = EXPAND;
		state_changed_dispatcher();
		refresh_size_request();
		show_all();
	}

	void Image::set_none_state() {
		image_state = NONE;
		state_changed_dispatcher();
		image->hide();
		refresh_size_request();
	}

	void Image::set_state(const ImageState new_state) {
		switch (new_state) {
		case NONE:
			set_none_state();
			break;
		case THUMBNAIL:
			set_thumb_state();
			break;
		case EXPAND:
		case FULL:
			if (!unscaled_image) {
				is_changing_state = true;
				fetch_image();
			} else {
				set_expand_state();
			}

			break;
		}
	}

	/* TODO: spoiler state
	 */
	bool Image::on_image_click(GdkEventButton *) {
		switch (image_state) {
		case NONE:
			break;
		case THUMBNAIL:
			if (!unscaled_image) {
				fetch_image();
				is_changing_state = true;
			} else {
				set_expand_state();
			}
			break;
		case EXPAND:
			// For now fall through, we don't have a 'Full' implementation
		case FULL:
			set_thumb_state();
			break;
		default:
			break;
		}

		return true;
	}

	void Image::refresh_size_request() {
		queue_resize();
		auto parent = get_parent();
		if ( parent ) {
			parent->queue_resize();
			queue_resize();
		}
	}

	void Image::set_new_scaled_image(const int width, const int height) {
		if (unscaled_image) {
			float scale = static_cast<float>(width) / static_cast<float>(post->get_width());
			int new_height = static_cast<int>(scale * post->get_height());
			int new_width = width;
			if ( new_height > height ) {
				scale = static_cast<float>(height) / static_cast<float>(post->get_height());
				new_height = height;
				new_width = static_cast<int>(scale * static_cast<float>(post->get_width()));
			}

			if (new_width > 0 && new_height > 0) {
				scaled_image = unscaled_image->scale_simple(new_width,
				                                            new_height,
				                                            Gdk::INTERP_BILINEAR);
			}

			image->set(scaled_image);
			scaled_height = new_height;
			scaled_width = new_width;
		}
	}

	Glib::RefPtr<Gdk::Pixbuf> Image::get_image() const {
		if (thumbnail_image)
			return thumbnail_image;
		else if (unscaled_animation)
			return unscaled_animation->get_static_image();
		else if (unscaled_image)
			return unscaled_image;
		else
			return Glib::RefPtr<Gdk::Pixbuf>();
	}

	Gtk::SizeRequestMode Image::get_request_mode_vfunc() const {
		return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
	}

	void Image::get_preferred_width_vfunc(int& minimum_width, int& natural_width) const {
		switch (image_state) {
		case NONE:
			minimum_width = 0;
			natural_width = 0;
			break;
		case THUMBNAIL:
			minimum_width = post->get_thumb_width();
			natural_width = post->get_thumb_width();
			break;
		case EXPAND:
			minimum_width = post->get_thumb_width();
			natural_width = post->get_width();
			break;
		case FULL:
			minimum_width = post->get_width();
			natural_width = post->get_width();
			break;
		}
	}
	
	void Image::get_preferred_height_vfunc(int& minimum_height, int& natural_height) const {
		switch (image_state) {
		case NONE:
			minimum_height = 0;
			natural_height = 0;
			break;
		case THUMBNAIL:
			minimum_height = post->get_thumb_height();
			natural_height = post->get_thumb_height();
			break;
		case EXPAND:
			minimum_height = post->get_thumb_height();
			natural_height = post->get_height();
			break;
		case FULL:
			minimum_height = post->get_height();
			natural_height = post->get_height();
			break;
		}
	}

	void Image::get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const {
		switch (image_state) {
		case NONE:
			minimum_height = 0;
			natural_height = 0;
			break;
		case THUMBNAIL:
			minimum_height = post->get_thumb_height();
			natural_height = post->get_thumb_height();
			break;
		case EXPAND:
			if ( width < post->get_width() ) {
				const float scale = static_cast<float>(width) / static_cast<float>(post->get_width());
				natural_height = static_cast<int>( scale * static_cast<float>(post->get_height()) );
			} else {
				natural_height = post->get_height();
			}
			minimum_height = natural_height;
			break;
		case FULL:
			g_warning("FULL not implemented.");
			if ( width < post->get_width() ) {
				const float scale = static_cast<float>(width) / static_cast<float>(post->get_width());
				minimum_height = static_cast<int>( scale * static_cast<float>(post->get_height()) );
			} else {
				minimum_height = post->get_height();
			}
			natural_height = minimum_height;
			break;
		}
	}
		
	void Image::get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const {
		switch (image_state) {
		case NONE:
			minimum_width = 0;
			natural_width = 0;
			break;
		case THUMBNAIL:
			minimum_width = post->get_thumb_width();
			natural_width = post->get_thumb_width();
			break;
		case EXPAND:
			minimum_width = post->get_thumb_width();
			if ( height < post->get_height() ) {
				const float scale = static_cast<float>(height) / static_cast<float>(post->get_height());
				natural_width = static_cast<int>( scale * static_cast<float>(post->get_width()) );
			} else {
				natural_width = post->get_width();
			}
			break;
		case FULL:
			if ( height < post->get_height() ) {
				const float scale = static_cast<float>(height) / static_cast<float>(post->get_height());
				minimum_width = static_cast<int>( scale * static_cast<float>(post->get_width()) );
			} else {
				minimum_width = post->get_width();
			}
			natural_width = minimum_width;
			break;
		}
	}

	void Image::on_size_allocate(Gtk::Allocation &allocation) {
		const int width = allocation.get_width();
		const int height = allocation.get_height();
		int used_width = 0;
		int used_height = 0;
		set_allocation(allocation);

		switch (image_state) {
		case NONE:
			break;
		case FULL:
		case THUMBNAIL:
			used_width = width;
			used_height = height;
			if ( !unscaled_image ) {
				break;
			}
		case EXPAND:
			if ( width < post->get_width() || height < post->get_height() ) {
				if (width != scaled_width) {
					set_new_scaled_image(width, height);					
				} else {
					image->set(scaled_image);
				}
				used_width = scaled_width;
				used_height = scaled_height;
				is_scaled = true;
			} else {
				if (unscaled_image)
					image->set(unscaled_image);
				else
					g_error("Image in EXPAND state without unscaled image data.");
				is_scaled = false;
				used_width = post->get_width();
				used_height = post->get_height();
			}
		}
		
		if (G_UNLIKELY(used_width > width || used_height > height)) {
			g_warning("Horizon::Image not allocated enough space!");
		}

		event_box->size_allocate(allocation);
		image->size_allocate(allocation);
	}
	
	void Image::forall_vfunc(gboolean, GtkCallback callback, gpointer callback_data) {
		if (event_box) callback(GTK_WIDGET(event_box->gobj()), callback_data);
		if (image)     callback(GTK_WIDGET(image    ->gobj()), callback_data);
	}

	GType Image::child_type_vfunc() const {
		return G_TYPE_NONE;
	}

	void Image::on_remove(Gtk::Widget*) {
	}
}
