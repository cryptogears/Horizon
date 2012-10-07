#ifndef SUMMARY_CELLRENDERER
#define SUMMARY_CELLRENDERER
#include <gtkmm/cellrenderer.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/cellrendererpixbuf.h>
#include <glibmm/property.h>
#include "thread_summary.hpp"


namespace Horizon {

	class SummaryCellRenderer : public Gtk::CellRenderer {
	public:
		virtual ~SummaryCellRenderer();
		SummaryCellRenderer();

		Glib::PropertyProxy< Glib::RefPtr<ThreadSummary> > property_threads();

	    bool is_activatable() const { return true; };

	protected:
		Glib::Property< Glib::RefPtr<ThreadSummary> > property_threads_;

		virtual Gtk::SizeRequestMode get_request_mode_vfunc() const G_GNUC_CONST;

		virtual void get_preferred_width_vfunc (Gtk::Widget& widget,
		                                        int& minimum_width,
		                                        int& natural_width) const;

		virtual void get_preferred_height_for_width_vfunc (Gtk::Widget& widget,
		                                                   int width,
		                                                   int& minimum_height,
		                                                   int& natural_height) const;

		virtual void get_preferred_height_vfunc (Gtk::Widget& widget,
		                                         int& minimum_height,
		                                         int& natural_height) const;

		virtual void get_preferred_width_for_height_vfunc (Gtk::Widget& widget,
		                                                   int height,
		                                                   int& minimum_width,
		                                                   int& natural_width) const;

		virtual void render_vfunc (const ::Cairo::RefPtr< ::Cairo::Context >& cr,
		                           Gtk::Widget& widget,
		                           const Gdk::Rectangle& background_area,
		                           const Gdk::Rectangle& cell_area,
		                           Gtk::CellRendererState flags);

	private:
		void on_changed();
		int thumb_natural_height;
		int thumb_natural_width;

		Gtk::CellRendererPixbuf cr_thumb;
		Gtk::CellRendererText cr_text;
		
	};

}



#endif
