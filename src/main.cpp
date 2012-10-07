#include "application.hpp"

int main (int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;

	Glib::RefPtr<Horizon::Application> rapp = Horizon::Application::create(Horizon::app_id); 
	rapp->run(argc, argv);
	
	return EXIT_SUCCESS;
}

