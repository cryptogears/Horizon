#include <iostream>
#include <libxml/parser.h>
#include "application.hpp"
#include "image_cache.hpp"

void init() __attribute__((constructor (101)));
void cleanup() __attribute__((destructor (101)));

void init() {
	xmlInitParser();
	curl_global_init(CURL_GLOBAL_ALL);
}

int main (int argc, char *argv[])
{
	Glib::RefPtr<Horizon::Application> rapp = Horizon::Application::create(Horizon::app_id); 
	rapp->run(argc, argv);
	
	return EXIT_SUCCESS;
}


void cleanup() {
	Horizon::ImageFetcher::cleanup();
	Horizon::ImageCache::cleanup();
	curl_global_cleanup();
	xmlCleanupParser();
}
