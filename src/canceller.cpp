#include "canceller.hpp"

namespace Horizon {
	Canceller::Canceller() :
		cancelled(false)
	{
	}

	void Canceller::cancel() {
		Glib::Threads::Mutex::Lock lock(mutex);
		cancelled = true;
	}

	void Canceller::involke_if_not_cancelled(std::function<void ()> functor) const {
		Glib::Threads::Mutex::Lock lock(mutex);
		if (!cancelled)
			functor();
	}

	bool Canceller::is_cancelled() const {
		Glib::Threads::Mutex::Lock lock(mutex);
		return cancelled;
	}
}
