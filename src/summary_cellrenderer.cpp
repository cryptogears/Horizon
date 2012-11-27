#include "summary_cellrenderer.hpp"
#include <iostream>
#include <iomanip>

namespace Horizon {
	
	SummaryCellRenderer::SummaryCellRenderer() :
		Glib::ObjectBase( typeid(SummaryCellRenderer) ),
		Gtk::CellRenderer(),
		property_threads_( *this, "threads" ),
		thumb_natural_height(0),
		thumb_natural_width(0)
	{
		property_threads_.get_proxy().signal_changed().connect(sigc::mem_fun(*this, &SummaryCellRenderer::on_changed));
		cr_text.set_alignment(.5, .5);
	}

	SummaryCellRenderer::~SummaryCellRenderer() {
		
	}
	
	void SummaryCellRenderer::on_changed() {
		auto ts = property_threads_.get_value();
		if (ts) {
			cr_thumb.property_pixbuf().set_value(ts->get_thumb_pixbuf());

			if ( ts->get_thumb_pixbuf() ) {
				thumb_natural_height = ts->get_thumb_pixbuf()->get_height();
				thumb_natural_width = ts->get_thumb_pixbuf()->get_width();
			}

			auto ppm = static_cast<float>(ts->get_reply_count()) /
				static_cast<float>(Glib::DateTime::
				                   create_now_utc().to_unix() -
				                   ts->get_unix_date());

			ppm = ppm * static_cast<float>(60);

			std::stringstream s;
			s << "R: <b>" << ts->get_reply_count() << "</b> I: <b>"
			  << ts->get_image_count() << "</b> PPM: <b>" 
			  << std::fixed << std::setprecision(2) << ppm << "</b>\n"
			  << ts->get_teaser();


			cr_text.property_markup().set_value(s.str());
			cr_text.property_wrap_mode().set_value(Pango::WRAP_WORD_CHAR);
			cr_text.property_wrap_width().set_value(250);
			cr_text.property_alignment().set_value(Pango::ALIGN_CENTER);
			cr_text.property_background().set_value("#8C92AC");
		}
	}

	Glib::PropertyProxy< Glib::RefPtr<ThreadSummary> > SummaryCellRenderer::property_threads() {
		return property_threads_.get_proxy();
	}

	Gtk::SizeRequestMode SummaryCellRenderer::get_request_mode_vfunc() const {
		return Gtk::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
	}

	void SummaryCellRenderer::get_preferred_width_vfunc (Gtk::Widget& widget,
	                                                     int& minimum_width,
	                                                     int& natural_width) const {
		Gtk::CellRenderer::get_preferred_width_vfunc(widget, minimum_width, natural_width);
		
		int text_min_width = minimum_width, text_nat_width = natural_width;

		cr_text.get_preferred_width(widget, text_min_width, text_nat_width);

		minimum_width = 260;// std::max(thumb_natural_width, text_min_width);
		natural_width = 260;//std::max(thumb_natural_width, text_nat_width);
	}

	void SummaryCellRenderer::get_preferred_height_for_width_vfunc (Gtk::Widget& widget,
	                                                                int width,
	                                                                int& minimum_height,
	                                                                int& natural_height) const {
		Gtk::CellRenderer::get_preferred_height_for_width_vfunc(widget, width, minimum_height, natural_height);

		int text_min_height = minimum_height, text_nat_height = natural_height;

		cr_text.get_preferred_height_for_width(widget, 260, text_min_height, text_nat_height);

		minimum_height = thumb_natural_height + text_min_height + 5;
		natural_height = thumb_natural_height + text_nat_height + 5;
	}

	void SummaryCellRenderer::get_preferred_height_vfunc (Gtk::Widget& widget,
	                                                      int& minimum_height,
	                                                      int& natural_height) const {
		Gtk::CellRenderer::get_preferred_height_vfunc(widget, minimum_height, natural_height);

		int text_min_height = minimum_height, text_nat_height = natural_height;

		cr_text.get_preferred_height(widget, text_min_height, text_nat_height);

		minimum_height = thumb_natural_height + text_min_height + 5;
		natural_height = thumb_natural_height + text_nat_height + 5;
	}

	void SummaryCellRenderer::get_preferred_width_for_height_vfunc (Gtk::Widget& widget,
	                                                                int height,
	                                                                int& minimum_width,
	                                                                int& natural_width) const {
		Gtk::CellRenderer::get_preferred_width_for_height_vfunc(widget, height, minimum_width, natural_width);

		//int text_min_width = minimum_width, text_nat_width = natural_width;

		//cr_text.get_preferred_width_for_height(widget,
		//                                     height - thumb_natural_height - 5,
		//                                     text_min_width,
		//                                     text_nat_width);

		minimum_width = 260; //+= std::max(thumb_natural_width, text_min_width);
		natural_width = 260;//+= std::max(thumb_natural_width, text_nat_width);
	}

	void SummaryCellRenderer::render_vfunc (const ::Cairo::RefPtr< ::Cairo::Context >& cr,
	                                        Gtk::Widget& widget,
	                                        const Gdk::Rectangle& background_area,
	                                        const Gdk::Rectangle& cell_area,
	                                        Gtk::CellRendererState flags) {
		Gtk::CellRenderer::render_vfunc(cr, widget, background_area, cell_area, flags);
		
		Gdk::Rectangle thumb_rec(cell_area.get_x(), cell_area.get_y(),
		                         cell_area.get_width(), thumb_natural_height);
		Gdk::Rectangle text_rec(cell_area.get_x(),
		                        cell_area.get_y() + thumb_natural_height + 5,
		                        cell_area.get_width(), cell_area.get_height() - thumb_natural_height - 5);
		
		cr_text.render(cr, widget, background_area, text_rec, flags);
		cr_thumb.render(cr, widget, background_area, thumb_rec, flags);
	}
	
}
