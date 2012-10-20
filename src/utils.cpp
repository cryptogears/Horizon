#include "utils.hpp"
#include <glibmm/exceptionhandler.h>

namespace Horizon {
	extern "C"
	{
		static void* call_thread_entry_slot(void* data);
	} //extern "C"
	
	static void* call_thread_entry_slot(void* data)
	{
		sigc::slot_base *const slot = reinterpret_cast<sigc::slot_base*>(data);

		try
			{
				// Recreate the specific slot, and drop the reference obtained by create().
				(*static_cast<sigc::slot<void>*>(slot))();
			}
		catch(Glib::Threads::Thread::Exit&)
			{
				// Just exit from the thread.  The Threads::Thread::Exit exception
				// is our sane C++ replacement of g_thread_exit().
			}
		catch(...)
			{
				Glib::exception_handlers_invoke();
			}

		delete slot;
		return 0;
	}

	Glib::Threads::Thread* create_named_thread(const std::string &name,
	                                           const sigc::slot<void> &slot) {
		  // Make a copy of slot on the heap
		sigc::slot_base *const slot_copy = new sigc::slot<void>(slot);

		GError* error = nullptr;

		if (name.size() > 16) {
			g_warning("Thread name %s is larger than 16 bytes", name.c_str());
		}

		GThread *const thread = g_thread_try_new(name.c_str(),
		                                         &call_thread_entry_slot,
		                                         slot_copy,
		                                         &error);
		
		if(error) {
			delete slot_copy;
			Glib::Error::throw_exception(error);
		}
		
		return reinterpret_cast<Glib::Threads::Thread*>(thread);
	}


}
