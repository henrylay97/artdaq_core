#include "artdaq-core/Utilities/TimeUtils.hh"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace BPT = boost::posix_time;

std::string artdaq::TimeUtils::
    convertUnixTimeToString(time_t inputUnixTime)
{
	// whole seconds
	BPT::ptime posixTime = BPT::from_time_t(inputUnixTime);
	std::string workingString = BPT::to_simple_string(posixTime);
	workingString.append(" UTC");

	return workingString;
}

std::string artdaq::TimeUtils::
    convertUnixTimeToString(struct timeval const& inputUnixTime)
{
	// deal with whole seconds first
	BPT::ptime posixTime = BPT::from_time_t(inputUnixTime.tv_sec);
	std::string workingString = BPT::to_simple_string(posixTime);

	// now fractional seconds
	char fractionalString[20];
	sprintf(fractionalString, "%06d", static_cast<int32_t>(inputUnixTime.tv_usec));
	workingString.append(".");
	workingString.append(fractionalString);
	workingString.append(" UTC");

	return workingString;
}

std::string artdaq::TimeUtils::
    convertUnixTimeToString(struct timespec const& inputUnixTime)
{
	// deal with whole seconds first
	BPT::ptime posixTime = BPT::from_time_t(inputUnixTime.tv_sec);
	std::string workingString = BPT::to_simple_string(posixTime);

	// now fractional seconds
	char fractionalString[20];
	sprintf(fractionalString, "%09ld", inputUnixTime.tv_nsec);
	workingString.append(".");
	workingString.append(fractionalString);
	workingString.append(" UTC");

	return workingString;
}

uint64_t artdaq::TimeUtils::gettimeofday_us()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

struct timespec artdaq::TimeUtils::get_realtime_clock()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return ts;
}

double artdaq::TimeUtils::
    convertUnixTimeToSeconds(time_t inputUnixTime)
{
	return static_cast<double>(inputUnixTime);
}

double artdaq::TimeUtils::
    convertUnixTimeToSeconds(struct timeval const& inputUnixTime)
{
	return static_cast<double>(inputUnixTime.tv_sec) + inputUnixTime.tv_usec / 1000000.0;
}

double artdaq::TimeUtils::
    convertUnixTimeToSeconds(struct timespec const& inputUnixTime)
{
	return static_cast<double>(inputUnixTime.tv_sec) + inputUnixTime.tv_nsec / 1000000000.0;  // Mr. Billion
}
