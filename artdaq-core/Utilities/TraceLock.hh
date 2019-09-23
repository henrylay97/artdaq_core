#ifndef ARTDAQ_CORE_UTILITIES_TRACELOCK
#define ARTDAQ_CORE_UTILITIES_TRACELOCK_HH 1

#include <mutex>
#include "tracemf.h"

/**
 * \brief The TraceLock class allows a user to debug the acquisition and releasing of locks, by wrapping the unique_lock<std::mutex> API with TRACE calls
 */
class TraceLock
{
public:
	/**
		 * \brief Construct a TraceLock
		 * \param mutex Mutex to hold lock on
		 * \param level Level to TRACE (in the TraceLock TRACE_NAME)
		 * \param description Description of lock (to be printed in TRACE calls)
		 */
	TraceLock(std::mutex& mutex, int level, std::string description);

	/**
		 * \brief Release the TraceLock
		 */
	virtual ~TraceLock();

private:
	std::unique_lock<std::mutex> lock_;
	std::string description_;
	int level_;
};

inline TraceLock::TraceLock(std::mutex& mutex, int level, std::string description)
    : lock_(mutex)
    , description_(description)
    , level_(level)
{
	TLOG_ARB(level_, "TraceLock") << "Acquired Lock " << description_ << ", mutex=" << (void*)&mutex << ", lock=" << (void*)&lock_;
}

inline TraceLock::~TraceLock()
{
	TLOG_ARB(level_, "TraceLock") << "Releasing lock " << description_ << ", lock=" << (void*)&lock_;
}

#endif
