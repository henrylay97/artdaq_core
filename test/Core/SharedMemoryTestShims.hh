#ifndef ARTDAQ_CORE_TEST_CORE_SHAREDMEMORYTESTSHIMS_HH
#define ARTDAQ_CORE_TEST_CORE_SHAREDMEMORYTESTSHIMS_HH

#include <random>
#include "artdaq-core/Utilities/TimeUtils.hh"

inline unsigned GetRandomKey(uint16_t identifier)
{
	static std::mt19937 rng(artdaq::TimeUtils::gettimeofday_us());
	static std::uniform_int_distribution<unsigned> gen(0x00000000, 0x0000FFFF);
	return gen(rng) + (identifier << 16) + getpid();
}

#endif