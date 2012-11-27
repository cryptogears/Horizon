#include "html_parser.hpp"
#include <iostream>
#include <map>

namespace Horizon {

	std::shared_ptr<HtmlParser> HtmlParser::getHtmlParser() {
		static auto ptr = std::shared_ptr<HtmlParser>(new HtmlParser());

		return ptr;
	}

	HtmlParser::HtmlParser() :
		is_OP_link(false),
		is_cross_thread_link(false),
		is_dead_link(false),
		is_code_tagged(false)
	{
		sax = g_new0(xmlSAXHandler, 1);
		sax->startElement = &horizon_html_parser_on_start_element;
		sax->endElement = &horizon_html_parser_on_end_element;
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
	
	std::list<gint64> HtmlParser::get_links(const std::string& html) {
		std::list<gint64> links;
		std::size_t pos = 0;
		std::size_t endpos = 0;
		
		while ( (pos = html.find("a href=", pos + 1)) != html.npos ) {
			endpos = html.find("quotelink", pos);
			if ( endpos != html.npos ) {
				std::size_t p = html.find("#p", pos);
				if ( p != html.npos  && p < endpos ) {
					gint64 link = g_ascii_strtoll(html.substr(p+2).c_str(), nullptr, 10);
					links.push_back(link);
				}
			}
		}

		return links;
	}

	std::list<Glib::ustring> HtmlParser::html_to_pango(const std::string &html, const gint64 id) {
		thread_id = id;
		built_string.clear();
		strings.clear();
		xmlFreeDoc(htmlCtxtReadMemory(ctxt,
		                              html.c_str(), 
		                              html.size(),
		                              NULL,
		                              "UTF-8",
		                              HTML_PARSE_RECOVER ));
		                              
		strings.push_back(built_string);
		return strings;
	}

	void horizon_html_parser_on_end_element(void* user_data,
	                                        const xmlChar* name) {

		HtmlParser *hp = static_cast<HtmlParser*>(user_data);
		std::stringstream s;
		s << reinterpret_cast<const char*>(name);
		std::string sname(s.str());

		if ( sname.size() == 1 &&
		     sname.find("a") != sname.npos ) {
			if (hp->is_OP_link) {
				hp->built_string.append(" (OP)");
			}
			if (hp->is_cross_thread_link) {
				hp->built_string.append(" (Cross-Thread)");
			}
			if (hp->is_dead_link) {
				hp->built_string.append(" (Dead)");
				hp->is_dead_link = false;
			}
			hp->built_string.append("</span></a>");
		} else if ( sname.find("span") != sname.npos ) {
			hp->built_string.append("</span>");
		} else if ( sname.find("html") != sname.npos ) {
		} else if ( sname.find("body") != sname.npos ) {
		} else if ( sname.find("br") != sname.npos ) {
		} else if ( sname.size() == 1 &&
		            sname.find("p") != sname.npos ) {
		} else if ( sname.find("pre") != sname.npos ) {
			hp->strings.push_back(hp->built_string);
			hp->built_string.clear();
			hp->is_code_tagged = false;
		} else if ( sname.compare("em") == 0 ) {
			hp->built_string.append("</i>");
		}

		else 
			std::cerr << "Debug: Unhandled tag \"/" << sname << "\"" << std::endl;
	}

	void horizon_html_parser_on_start_element(void* user_data,
	                                          const xmlChar* name,
	                                          const xmlChar** attrs) {
		HtmlParser* hp = static_cast<HtmlParser*>(user_data);
		if (name != nullptr) {
			try {
				Glib::ustring sname( reinterpret_cast<const char*>(name) );
				std::map<Glib::ustring, Glib::ustring> sattrs;
				std::stringstream stream;

				if (attrs != nullptr) {
					for ( int i = 0; attrs[i] != nullptr; i += 2) {
						const Glib::ustring key(reinterpret_cast<const char*>(attrs[i]));
						if ( attrs[i+1] != nullptr ) {
							const Glib::ustring value(reinterpret_cast<const char*>(attrs[i+1]));
							sattrs.insert(std::make_pair(std::move(key), std::move(value)));
						} else {
							sattrs.insert(std::make_pair(std::move(key), ""));
							break;
						}
					}
				}

				if ( sname.find("body") != sname.npos ) {
				} else if ( sname.find("html") != sname.npos ) {
				} else if ( sname.size() == 1 &&
				            sname.find("p") != sname.npos ) {
				} else if ( sname.find("br") != Glib::ustring::npos ) {
					hp->built_string.append("\n");
				} else if ( sname.size() == 1 && 
				            sname.find("a") != Glib::ustring::npos &&
				            sattrs.count("class") == 1 &&
				            sattrs["class"].find("quotelink") != Glib::ustring::npos &&
				            sattrs.count("href") == 1) {
					auto const url     = sattrs["href"];
					auto const offset  = url.find_last_of("p") + 1;
					auto const postnum = sattrs["href"].substr(offset);
					stream << "<a href=\"" << url << "\">"
					       << "<span color=\"#D00\">";
					hp->built_string.append(stream.str());

					auto parent_offset = offset - 3;
					while (parent_offset > 0 && g_ascii_isdigit(url.c_str()[parent_offset]))
						--parent_offset;
					if (parent_offset != 0)
						++parent_offset;

					auto const link_parent_id = g_ascii_strtoll(url.substr(parent_offset).c_str(),
					                                            nullptr,
					                                            10);
					auto const link_post_id = g_ascii_strtoll(postnum.c_str(),
					                                          nullptr,
					                                          10);

					if ( link_post_id == link_parent_id )
						hp->is_OP_link = true;
					else
						hp->is_OP_link = false;
					if ( link_parent_id == hp->thread_id )
						hp->is_cross_thread_link = false;
					else
						hp->is_cross_thread_link = true;
				} else if (sname.find("span") != sname.npos &&
				           sattrs.count("class") == 1 &&
				           sattrs["class"].find("spoiler") != Glib::ustring::npos ) {
					stream << "<span color=\"#000\" background=\"#000\">";
					hp->built_string.append(stream.str());

				} else if ( sname.find("span") != sname.npos &&
				            sattrs.count("class") == 1 &&
				            sattrs["class"].find("quote") != sname.npos ) {
					stream << "<span color=\"#789922\">";
					hp->built_string.append(stream.str());
				} else if ( sname.find("pre") != sname.npos &&
				            sattrs.count("class") == 1 &&
				            sattrs["class"].find("prettyprint") != Glib::ustring::npos ) {
					hp->strings.push_back(hp->built_string);
					hp->built_string.clear();
					hp->is_code_tagged = true;
				} else if ( sname.compare("a") == 0 &&
				            sattrs.find("href") != sattrs.end() ) {
					stream << "<a href=\"" << sattrs["href"] << "\"><span color=\"#34345C\">";
					hp->built_string.append(stream.str());
				} else if ( sname.compare("span") == 0 &&
				            sattrs.count("class") == 1 &&
				            sattrs["class"].compare("deadlink") == 0) {
					stream << "<span>";
					hp->is_dead_link = true;
					hp->built_string.append(stream.str());
				} else if (sname.compare("span") == 0) {
					stream << "<span>";
					std::cerr << "Debug: Unhandled span attributes: ";
					for ( auto pair : sattrs )
						std::cerr << "\"" << pair.first << "\" = \"" << pair.second << "\" ";
					std::cerr << std::endl;					
					hp->built_string.append(stream.str());
				} else if (sname.compare("em") == 0) {
					stream << "<i>";
					hp->built_string.append(stream.str());
				}

				else {
					std::cerr << "Debug: Unhandled tag \"" << name << "\"";
					if (sattrs.size() > 0) {
						std::cerr << " with attribute";
						if (sattrs.size() > 1)
							std::cerr << "s";
						std::cerr << ": ";
					
						for ( auto pair : sattrs )
							std::cerr << "\"" << pair.first << "\" = \"" << pair.second << "\" ";
					}
					std::cerr << std::endl;					
				}
			} catch (std::exception e) {
				std::cerr << "Error startElement casting to string: " << e.what()
				          << std::endl;
			}
		}
	}

	void horizon_html_parser_on_characters(void* user_data,
	                                       const xmlChar* chars,
	                                       int size) {
		HtmlParser* hp = static_cast<HtmlParser*>(user_data);
		if (!hp->is_code_tagged) {
			gchar* cstr = g_markup_escape_text(reinterpret_cast<const gchar*>(chars), size);
			hp->built_string.append(Glib::ustring(cstr));
			g_free(cstr);
		} else {
			std::string str(reinterpret_cast<const char*>(chars), size);
			hp->built_string.append(str);
		}
	}

	void horizon_html_parser_on_xml_error(void*, xmlErrorPtr error) {
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
