#include "artdaq-core/Utilities/TimeUtils.hh"

#define BOOST_TEST_MODULE TimeUtils_t
#include <cmath>
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "TimeUtils_t"
#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(TimeUtils_test)

BOOST_AUTO_TEST_CASE(GetElapsedTime)
{
	auto then = std::chrono::steady_clock::now();
	auto now = then + std::chrono::seconds(1);

	BOOST_REQUIRE_EQUAL(artdaq::TimeUtils::GetElapsedTime(then, now), 1);
	BOOST_REQUIRE_EQUAL(artdaq::TimeUtils::GetElapsedTimeMilliseconds(then, now), 1000);
	BOOST_REQUIRE_EQUAL(artdaq::TimeUtils::GetElapsedTimeMicroseconds(then, now), 1000000);

	auto start = std::chrono::steady_clock::now();
	for (int ii = 0; ii < 1000000; ++ii)
	{
		artdaq::TimeUtils::GetElapsedTime(start);
	}
	auto dur = artdaq::TimeUtils::GetElapsedTime(start);
	TLOG(TLVL_INFO) << "Time to call GetElapsedTime 1000000 times: " << dur << " s ( ave: " << dur / 1000000 << " s/call ).";
	start = std::chrono::steady_clock::now();
	for (int ii = 0; ii < 1000000; ++ii)
	{
		artdaq::TimeUtils::GetElapsedTimeMilliseconds(start);
	}
	dur = artdaq::TimeUtils::GetElapsedTime(start);
	TLOG(TLVL_INFO) << "Time to call GetElapsedTimeMilliseconds 1000000 times: " << dur << " s ( ave: " << dur / 1000000 << " s/call ).";
	start = std::chrono::steady_clock::now();
	for (int ii = 0; ii < 1000000; ++ii)
	{
		artdaq::TimeUtils::GetElapsedTimeMicroseconds(start);
	}
	dur = artdaq::TimeUtils::GetElapsedTime(start);
	TLOG(TLVL_INFO) << "Time to call GetElapsedTimeMicroseconds 1000000 times: " << dur << " s ( ave: " << dur / 1000000 << " s/call ).";
}

BOOST_AUTO_TEST_CASE(UnixTime)
{
	time_t t = time(0);
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	struct timespec ts = artdaq::TimeUtils::get_realtime_clock();

	auto timeString = artdaq::TimeUtils::convertUnixTimeToString(t);
	TLOG(TLVL_INFO) << "time_t to string: " << timeString;
	auto timeDouble = artdaq::TimeUtils::convertUnixTimeToSeconds(t);
	TLOG(TLVL_INFO) << "time_t to seconds: " << timeDouble;

	auto valString = artdaq::TimeUtils::convertUnixTimeToString(tv);
	TLOG(TLVL_INFO) << "timeval to string: " << valString;
	auto valDouble = artdaq::TimeUtils::convertUnixTimeToSeconds(tv);
	TLOG(TLVL_INFO) << "timeval to seconds: " << valDouble;

	auto specString = artdaq::TimeUtils::convertUnixTimeToString(ts);
	TLOG(TLVL_INFO) << "timespec to string: " << specString;
	auto specDouble = artdaq::TimeUtils::convertUnixTimeToSeconds(ts);
	TLOG(TLVL_INFO) << "timespec to seconds: " << specDouble;

	BOOST_REQUIRE_EQUAL(timeDouble, std::floor(valDouble));
	BOOST_REQUIRE_EQUAL(timeDouble, std::floor(specDouble));
}

BOOST_AUTO_TEST_CASE(GetTimeOfDayUS)
{
	auto now = artdaq::TimeUtils::gettimeofday_us();
	struct timespec ts = artdaq::TimeUtils::get_realtime_clock();
	BOOST_REQUIRE_EQUAL(now / 1000000, ts.tv_sec);
}

BOOST_AUTO_TEST_SUITE_END()