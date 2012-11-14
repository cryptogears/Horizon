#include <iostream>
#include <libxml/parser.h>
#include <glibmm/init.h>
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
	
	return EXIT_SUCCESS;
}


void cleanup() {
	Horizon::ImageFetcher::cleanup();
	Horizon::ImageCache::cleanup();
	curl_global_cleanup();
	xmlCleanupParser();
}
