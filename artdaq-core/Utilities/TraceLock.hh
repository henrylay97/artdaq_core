#ifndef ARTDAQ_CORE_UTILITIES_TRACELOCK
#define ARTDAQ_CORE_UTILITIES_TRACELOCK_HH 1

#include <mutex>
#include "TRACE/tracemf.h"

/**
 * \brief The TraceLock class allows a user to debug the acquisition and releasing of locks, by wrapping the unique_lock<std::mutex> API with TRACE calls
 */
template<typename MUTEX = std::mutex>
class TraceLock
{
public:
	/**
		 * \brief Construct a TraceLock
		 * \param mutex Mutex to hold lock on
		 * \param level Level to TRACE (in the TraceLock TRACE_NAME)
		 * \param description Description of lock (to be printed in TRACE calls)
		 */
	TraceLock(MUTEX& mutex, int level, std::string const& description)
	    : lock_(mutex)
	    , description_(description)
	    , level_(level)
	{
		TLOG_ARB(level_, "TraceLock") << "Acquired Lock " << description_ << ", mutex=" << (void*)&mutex << ", lock=" << (void*)&lock_;  // NOLINT(google-readability-casting)
	}

	/**
		 * \brief Release the TraceLock
		 */
	virtual ~TraceLock()
	{
		TLOG_ARB(level_, "TraceLock") << "Releasing lock " << description_ << ", lock=" << (void*)&lock_;  // NOLINT(google-readability-casting)
	}

private:
	TraceLock(TraceLock const&) = delete;
	TraceLock(TraceLock&&) = delete;
	TraceLock& operator=(TraceLock const&) = delete;
	TraceLock& operator=(TraceLock&&) = delete;

	std::unique_lock<MUTEX> lock_;
	std::string description_;
	int level_;
};

#endif
