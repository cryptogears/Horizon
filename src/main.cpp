#include <iostream>
#include <libxml/parser.h>
#include <glibmm/init.h>
#include <pangomm/cairofontmap.h>
#include "application.hpp"
#include "image_cache.hpp"


void init() __attribute__((constructor (101)));
void cleanup() __attribute__((destructor (101)));

void init() {
	xmlInitParser();
	curl_global_init(CURL_GLOBAL_ALL);
	Glib::init();
}

int main (int argc, char *argv[])
{
	std::unique_ptr<Horizon::Application> rapp(new Horizon::Application(Horizon::app_id));
	rapp->run(argc, argv);
	rapp.reset();

	return EXIT_SUCCESS;
}


void cleanup() {
	Horizon::ImageFetcher::cleanup();

	/*
	 * This is only required to clean up all memory before exit.
	 */
	pango_cairo_font_map_set_default(nullptr);
	curl_global_cleanup();
	xmlCleanupParser();
}
