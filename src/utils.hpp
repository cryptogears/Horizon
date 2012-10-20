#ifndef HORIZON_UTILS_HPP
#define HORIZON_UTILS_HPP
#include <glibmm/threads.h>

namespace Horizon {

	Glib::Threads::Thread* create_named_thread(const std::string &name,
	                                           const sigc::slot<void> &slot);
}

#endif
