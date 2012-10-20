#ifndef HORIZON_IMAGE_HPP
#define HORIZON_IMAGE_HPP

#include <gtkmm/container.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/image.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/grid.h>
#include "image_fetcher.hpp"
#include "thread.hpp"
#include <sigc++/functors/slot.h>

namespace Horizon {

	class Image : public Gtk::Container {
	public:
		enum ImageState { NONE, THUMBNAIL, EXPAND, FULL };
		static std::shared_ptr<Image> create(const Glib::RefPtr<Post>&,
		                                     sigc::slot<void, const ImageState&>);
		~Image() = default;

		Glib::RefPtr<Gdk::Pixbuf> get_image() const;

		void set_state(const ImageState new_state);

	protected:
		Image(const Glib::RefPtr<Post>&,
		      sigc::slot<void, const Image::ImageState&>);

		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const;
		virtual void get_preferred_width_vfunc(int& minimum_width, int& natural_width) const;
		virtual void get_preferred_height_for_width_vfunc(int width, int& minimum_height, int& natural_height) const;
		virtual void get_preferred_height_vfunc(int& minimum_height, int& natural_height) const;
		virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const;
		virtual void on_size_allocate(Gtk::Allocation& allocation);
		virtual void forall_vfunc(gboolean include_internals, GtkCallback callback, gpointer callback_data);
		virtual GType child_type_vfunc() const;

	private:
		Glib::RefPtr<Post> post;
		Gtk::Grid  *grid;
		Gtk::EventBox event_box;
		Gtk::Image image;

		ImageState image_state;
		sigc::signal<void, const ImageState&> signal_state_changed;
		Glib::Dispatcher state_changed_dispatcher;
		void run_state_changed_callbacks() const;

		Glib::RefPtr<Gdk::Pixbuf> thumbnail_image;
		Glib::RefPtr<Gdk::Pixbuf> unscaled_image;
		Glib::RefPtr<Gdk::Pixbuf> scaled_image;
		Glib::RefPtr<Gdk::PixbufAnimation> unscaled_animation;
		Glib::RefPtr<Gdk::PixbufAnimationIter> animation_iter;
		Glib::TimeVal animation_time;
		bool on_animation_timeout();
		int scaled_width, scaled_height;
		bool is_scaled;

		sigc::connection animation_timeout;
		void on_image_unmap();
		void on_image_unrealize();
		bool on_image_draw(const Cairo::RefPtr< Cairo::Context > &cr);
		void reset_animation_iter();

		void fetch_thumbnail();
		void fetch_image();
		void on_thumb_ready(std::string hash);
		void on_image_ready(std::string hash);
		void set_none_state();
		void set_thumb_state();
		void set_expand_state();
		sigc::connection thumb_connection;
		sigc::connection image_connection;
		std::shared_ptr<ImageFetcher> ifetcher;

		void refresh_size_request();
		void set_new_scaled_image(const int width, const int height);
		bool on_image_click(GdkEventButton* btn);
		
	};

}

#endif
