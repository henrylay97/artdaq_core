#ifndef artdaq_core_Utilities_TimeUtils_h
#define artdaq_core_Utilities_TimeUtils_h

#include <sys/time.h>
#include <chrono>
#include <string>

namespace artdaq {
/**
 * \brief Namespace to hold useful time-converting functions
 */
namespace TimeUtils {
/**
		* We shall use artdaq::detail::seconds as our "standard" duration
		* type. Note that this differs from std::chrono::seconds, which has
		* a representation in some integer type of at least 35 bits.
		*
		* daqrate::duration dur(1.0) represents a duration of 1 second.
		* daqrate::duration dur2(0.001) represents a duration of 1
		* millisecond.
		*/
typedef std::chrono::duration<double, std::ratio<1>> seconds;

/// <summary>
/// Get the number of seconds in the given interval
/// </summary>
/// <param name="then">std::chrono::steady_clock::time_point representing start of interval</param>
/// <param name="now">std::chrono::steady_clock::time_point representing end of interval. Defaults to std::chrono::steady_clock::now()</param>
/// <returns>Seconds in time interval, expressed as double</returns>
inline constexpr double GetElapsedTime(std::chrono::steady_clock::time_point then, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
{
	return std::chrono::duration_cast<seconds>(now - then).count();
}

/// <summary>
/// Gets the number of microseconds in the given time interval
/// </summary>
/// <param name="then">std::chrono::steady_clock::time_point representing start of interval</param>
/// <param name="now">std::chrono::steady_clock::time_point representing end of interval. Defaults to std::chrono::steady_clock::now()</param>
/// <returns>Microseconds in time interval</returns>
inline constexpr size_t GetElapsedTimeMicroseconds(std::chrono::steady_clock::time_point then, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
{
	return static_cast<size_t>(GetElapsedTime(then, now) * 1000000);
}

/// <summary>
/// Gets the number of milliseconds in the given time interval
/// </summary>
/// <param name="then">std::chrono::steady_clock::time_point representing start of interval</param>
/// <param name="now">std::chrono::steady_clock::time_point representing end of interval. Defaults to std::chrono::steady_clock::now()</param>
/// <returns>Milliseconds in time interval</returns>
inline constexpr size_t GetElapsedTimeMilliseconds(std::chrono::steady_clock::time_point then, std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
{
	return static_cast<size_t>(GetElapsedTime(then, now) * 1000);
}

/**
	   * \brief Converts a Unix time to its string representation, in UTC
	   * \param inputUnixTime A time_t Unix time variable
	   * \return std::string representation of Unix time, in UTC
	   */
std::string convertUnixTimeToString(time_t inputUnixTime);

/**
		* \brief Converts a Unix time to its string representation, in UTC
		* \param inputUnixTime A struct timeval Unix time variable
		* \return std::string representation of Unix time, in UTC
		*/
std::string convertUnixTimeToString(struct timeval const& inputUnixTime);

/**
		* \brief Converts a Unix time to its string representation, in UTC
		* \param inputUnixTime A struct timespec Unix time variable
		* \return std::string representation of Unix time, in UTC
		*/
std::string convertUnixTimeToString(struct timespec const& inputUnixTime);

/**
		* \brief Get the current time of day in microseconds (from gettimeofday system call)
		* \return The current time of day in microseconds
		*/
uint64_t gettimeofday_us();

/**
		 * \brief Get the current time of day as a pair of seconds and nanoseconds (from clock_gettime(CLOCK_REALTIME, ...) system call)
		 * \return Pair of seconds, nanoseconds wallclock time
		 */
struct timespec get_realtime_clock();

/**
		* \brief Converts a Unix time to double
		* \param inputUnixTime A time_t Unix time variable
		* \return double representation of Unix time (seconds since epoch)
		*/
double convertUnixTimeToSeconds(time_t inputUnixTime);

/**
		* \brief Converts a Unix time to double
		* \param inputUnixTime A struct timeval Unix time variable
		* \return double representation of Unix time (in seconds)
		*/
double convertUnixTimeToSeconds(struct timeval const& inputUnixTime);

/**
		* \brief Converts a Unix time to double
		* \param inputUnixTime A struct timespec Unix time variable
		* \return double representation of Unix time (in seconds)
		*/
double convertUnixTimeToSeconds(struct timespec const& inputUnixTime);
}  // namespace TimeUtils
}  // namespace artdaq

#endif /* artdaq_core_Utilities_TimeUtils_h */

// Local Variables:
// mode: c++
// End:
