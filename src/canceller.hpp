#ifndef CANCELLER_HPP
#define CANCELLER_HPP
#include <functional>
#include <glibmm/threads.h>

namespace Horizon {

	class Canceller {
	public:
		Canceller();
		~Canceller() = default;
		Canceller(const Canceller&) = delete;
		Canceller& operator=(const Canceller&) = delete;

		void cancel();
		bool is_cancelled() const;
		void involke_if_not_cancelled(std::function<void ()>) const;

	private:
		mutable Glib::Threads::Mutex mutex;
		bool cancelled;
	};
}

#endif
