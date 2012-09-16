#include "application.hpp"

int main (int argc, char *argv[])
{
	g_type_init ();
	
	Glib::RefPtr<Horizon::Application> rapp = Horizon::Application::create(argc, argv); 
	rapp->run();
	
  return EXIT_SUCCESS;
}

