#ifndef HTML_PARSER_HPP
#define HTML_PARSER_HPP

#include <memory>
#include <glibmm/ustring.h>
#include <libxml/HTMLparser.h>

namespace Horizon {

	class HtmlParser {
	public:
		static std::shared_ptr<HtmlParser> getHtmlParser();
		~HtmlParser();

		Glib::ustring html_to_pango(const std::string& html);

	protected:
		HtmlParser();

	private:
		htmlSAXHandlerPtr sax;
		xmlParserCtxtPtr ctxt;
		Glib::ustring built_string;

		friend void horizon_html_parser_on_start_element(void* user_data, const xmlChar* name, const xmlChar** attrs);
		friend void horizon_html_parser_on_characters(void* user_data, const xmlChar* chars, int len);
		friend void horizon_html_parser_on_xml_error(void* user_data, xmlErrorPtr error);
	};

	void horizon_html_parser_on_start_element(void* user_data, const xmlChar* name, const xmlChar** attrs);
	void horizon_html_parser_on_characters(void* user_data, const xmlChar* chars, int len);
	void horizon_html_parser_on_xml_error(void* user_data, xmlErrorPtr error);
	size_t horizon_html_parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);

}



#endif
