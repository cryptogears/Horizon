
#include "horizon_image.hpp"
#include <iostream>
#include <glibmm/main.h>

namespace Horizon {

	std::shared_ptr<Image> Image::create(const Glib::RefPtr<Post> &post, Gtk::Grid *grid) {
		return std::shared_ptr<Image>(new Image(post, grid));
	}
	
	Image::Image(const Glib::RefPtr<Post> &post_, Gtk::Grid* parent) :
		Gtk::Container(),
		post(post_),
		grid(parent),
		image_state(NONE),
		scaled_width(-1),
		scaled_height(-1),
		ifetcher(ImageFetcher::get())
	{
		set_has_window(false);
		set_redraw_on_allocate(false);
		set_name("imagewindow");
		event_box.set_events(Gdk::BUTTON_PRESS_MASK);
		event_box.set_visible_window(false);
		event_box.signal_button_press_event().connect( sigc::mem_fun(*this, &Image::on_image_click) );
		image.set_name("image");
		image.set_halign(Gtk::ALIGN_START);
		image.set_valign(Gtk::ALIGN_START);
		event_box.set_parent(*this);
		image.set_parent(*this);

		if (post->has_image()) {
			fetch_thumbnail();
		} else {
			g_error("Horizon::Image created for post %d that has no image.",
			        post->get_id()); 
		}
	}

	void Image::fetch_thumbnail() {
		if ( ifetcher->has_thumb(post->get_hash()) ) {
			on_thumb_ready(post->get_hash());
		} else {
			thumb_connection = ifetcher->signal_thumb_ready.connect(sigc::mem_fun(*this, &Image::on_thumb_ready));
			ifetcher->download_thumb(post->get_hash(), post->get_thumb_url());
		}
	}

	void Image::fetch_image() {
		if ( unscaled_animation ||
		     unscaled_image ||
		     ifetcher->has_animation(post->get_hash()) ||
		     ifetcher->has_image(post->get_hash()) ) {
			on_image_ready(post->get_hash());
			return;
		} else {
			image_connection = ifetcher->signal_image_ready.connect(sigc::mem_fun(*this, &Image::on_image_ready));
			ifetcher->download_image(post->get_hash(),
			                         post->get_image_url(),
			                         post->get_image_ext());
		}
	}

	void Image::on_thumb_ready(std::string hash) {
		if ( post->get_hash().find(hash) == std::string::npos )
			return;
		if ( !thumbnail_image ) {
			if (thumb_connection.connected())
				thumb_connection.disconnect();
			thumbnail_image = ifetcher->get_thumb(hash);
			if (G_UNLIKELY(!thumbnail_image)) {
				g_error("Received pixbuf thumb is null");
			}
		}

		set_thumb_state();
	}

	void Image::set_thumb_state() {
		try {
			Gtk::Widget* comment = nullptr;
			if (image_state == NONE) {
				comment = grid->get_child_at(0, 0);
				grid->remove(*comment);
				grid->attach(*this, 0, 0, 1, 1);
			} else {
				comment = grid->get_child_at(0, 1);
				grid->remove(*comment);
			}
			if (comment != nullptr) {
				image_state = THUMBNAIL;
				if (!unscaled_image)
					image.set(thumbnail_image);
				grid->attach_next_to(*comment, *this, Gtk::POS_RIGHT, 1, 1);
				refresh_size_request();
				show_all();
			} else {
				g_warning("Comment not in expected location on grid!");
			}
		} catch (Glib::Error e) {
			g_warning("Error creating image from pixmap: %s", e.what().c_str());
		}
	}

	void Image::on_animation_timeout() {
		animation_time.assign_current_time();
		if (animation_iter->advance(animation_time)) {
			unscaled_image = animation_iter->get_pixbuf();
			if (is_scaled) {
				set_new_scaled_image(scaled_width, scaled_height);
			} else {
				image.set(unscaled_image);
			}
		}

		Glib::signal_timeout().connect_once(sigc::mem_fun(*this, &Image::on_animation_timeout),
		                                     animation_iter->get_delay_time());
	}

	void Image::on_image_ready(std::string hash) {
		if ( post->get_hash().find(hash) == std::string::npos ) 
			return;
		if (post->is_gif()) {
			if ( ! unscaled_animation ) {
				if (image_connection.connected())
					image_connection.disconnect();
				auto animation = ifetcher->get_animation(hash);
				if (animation->is_static_image()) {
					unscaled_image = animation->get_static_image();
				} else {
					unscaled_animation = animation;
					animation_time.assign_current_time();
					animation_iter = unscaled_animation->get_iter(&animation_time);
					unscaled_image = animation_iter->get_pixbuf();
					Glib::signal_timeout().connect_once(sigc::mem_fun(*this, &Image::on_animation_timeout),
					                                     animation_iter->get_delay_time());
				}
			}
		} else {
			if ( ! unscaled_image ) {
				if (image_connection.connected())
					image_connection.disconnect();
				unscaled_image = ifetcher->get_image(hash);
			}
		}

		if (G_UNLIKELY( !unscaled_animation && !unscaled_image )) {
			g_error("Received pixbuf image is null");
			return;
		}

		set_expand_state();
	}

	void Image::set_expand_state() {
		if (image_state != THUMBNAIL) {
			g_error("Image_state not thumbnail, aborting.");
			return;
		}

		Gtk::Widget* comment = grid->get_child_at(1, 0);
		if (comment) {
			image_state = EXPAND;
			grid->remove(*comment);
			grid->attach_next_to(*comment, *this, Gtk::POS_BOTTOM, 1, 1);
			refresh_size_request();
			show_all();
		} else {
			g_error("Comment label not at expected location in grid");
		}
	}

	bool Image::on_image_click(GdkEventButton *btn) {
		switch (image_state) {
		case NONE:
			break;
		case THUMBNAIL:
			if (!unscaled_image)
				fetch_image();
			else
				set_expand_state();
			break;
		case EXPAND:
			// For now fall through, we don't have a 'Full' implementation
		case FULL:
			set_thumb_state();
			break;
		default:
			break;
		}

	}

	void Image::refresh_size_request() {
		queue_resize();
		grid->queue_resize();
		queue_resize();
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

			scaled_image = unscaled_image->scale_simple(new_width,
			                                            new_height,
			                                            Gdk::INTERP_BILINEAR);
			image.set(scaled_image);
			scaled_height = new_height;
			scaled_width = new_width;
		}
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
			if ( width < post->get_width() ) {
				if (width != scaled_width) {
					set_new_scaled_image(width, height);					
				} else {
					image.set(scaled_image);
				}
				used_width = scaled_width;
				used_height = scaled_height;
				is_scaled = true;
			} else {
				if (unscaled_image)
					image.set(unscaled_image);
				else
					g_error("Image in EXPAND state without unscaled image data.");
				is_scaled = false;
				used_width = post->get_width();
				used_height = post->get_height();
			}
		}
		
		if (G_UNLIKELY(used_width > width || used_height > height)) {
			g_error("Horizon::Image not allocated enough space!");
		}

		event_box.size_allocate(allocation);
		image.size_allocate(allocation);
	}

	void Image::forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data) {
		callback(GTK_WIDGET(event_box.gobj()), callback_data);
		callback(GTK_WIDGET(image.gobj()), callback_data);
	}

	GType Image::child_type_vfunc() const {
		return G_TYPE_NONE;
	}
}
