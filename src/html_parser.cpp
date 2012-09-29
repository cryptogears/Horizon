#include "html_parser.hpp"
#include <iostream>

namespace Horizon {

	std::shared_ptr<HtmlParser> HtmlParser::getHtmlParser() {
		static auto ptr = std::shared_ptr<HtmlParser>(new HtmlParser());

		return ptr;
	}

	HtmlParser::HtmlParser()
	{
		sax = g_new0(xmlSAXHandler, 1);
		sax->startElement = &horizon_html_parser_on_start_element;
		sax->characters = &horizon_html_parser_on_characters;
		sax->serror = &horizon_html_parser_on_xml_error;
		sax->initialized = XML_SAX2_MAGIC;

		ctxt = htmlCreatePushParserCtxt(sax, this, NULL, 0, NULL,
		                                XML_CHAR_ENCODING_UTF8);
		if (G_UNLIKELY( ctxt == NULL )) {
			g_error("Unable to create libxml2 HTML context!");
		}
	}

	HtmlParser::~HtmlParser() {
		htmlFreeParserCtxt(ctxt);
		g_free(sax);
	}

	Glib::ustring HtmlParser::html_to_pango(const std::string &html) {
		built_string.clear();

		xmlFreeDoc(htmlCtxtReadMemory(ctxt,
		                              html.c_str(), 
		                              html.size(),
		                              NULL,
		                              "UTF-8",
		                              HTML_PARSE_RECOVER ));
		                              
		return built_string;
	}

	void horizon_html_parser_on_start_element(void* user_data,
	                                          const xmlChar* name,
	                                          const xmlChar** attrs) {
		HtmlParser* hp = static_cast<HtmlParser*>(user_data);

		try {
			Glib::ustring str( reinterpret_cast<const char*>(name) );

			if ( str.find("br") != Glib::ustring::npos ) {
				hp->built_string.append("\n");
			} else {
				if (false) {
					std::cerr << "Ignoring tag " << name;
					if (attrs != NULL) {
						std::cerr << " attrs:";
						for ( int i = 0; attrs[i] != NULL; i++) {
							std::cerr << " " << attrs[i];
					}
					}
					std::cerr << std::endl;
				}
			}
		} catch (std::exception e) {
			std::cerr << "Error startElement casting to string: " << e.what()
			          << std::endl;
		}
	}

	void horizon_html_parser_on_characters(void* user_data,
	                                       const xmlChar* chars,
	                                       int size) {
		HtmlParser* hp = static_cast<HtmlParser*>(user_data);
		gchar* cstr = g_markup_escape_text(reinterpret_cast<const gchar*>(chars), size);
		hp->built_string.append(static_cast<char*>(cstr));
		g_free(cstr);
	}

	void horizon_html_parser_on_xml_error(void* user_data, xmlErrorPtr error) {
		HtmlParser* hp = static_cast<HtmlParser*>(user_data);
		switch (error->code) {
		case XML_ERR_INVALID_ENCODING:
			std::cerr << "Warning: Invalid UTF-8 encoding. " 
			          << "It can't be helped." << std::endl;
			break;
		case XML_ERR_NAME_REQUIRED:
		case XML_ERR_TAG_NAME_MISMATCH:
		case XML_ERR_ENTITYREF_SEMICOL_MISSING:
		case XML_ERR_INVALID_DEC_CHARREF:
		case XML_ERR_LTSLASH_REQUIRED:
		case XML_ERR_GT_REQUIRED:
			// Ignore
			break;
		default:
			if (error->domain == XML_FROM_HTML) {
				std::cerr << "Warning: Got unexpected libxml2 HTML error code "
				          << error->code << ". This is probably ok : " 
				          << error->message;
			} else {
				std::cerr << "Error: Got unexpected libxml2 error code "
				          << error->code << " from domain " << error->domain
				          << " which means: " << error->message << std::endl;
				std::cerr << "\tPlease report this error to the developer."
				          << std::endl;
			}
		}
	}
}
