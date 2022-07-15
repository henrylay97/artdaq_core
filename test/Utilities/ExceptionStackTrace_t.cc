#include "artdaq-core/Utilities/ExceptionStackTrace.hh"

#define BOOST_TEST_MODULE ExceptionStackTrace_t
#include "cetlib/quiet_unit_test.hpp"

#define TRACE_NAME "ExceptionStackTrace_t"
#include "TRACE/tracemf.h"

BOOST_AUTO_TEST_SUITE(ExceptionStackTrace_test)

/**
 * @brief Print the Exception Stack Trace
*/
void PrintExceptionStackTrace()
{
	auto message = artdaq::debug::getStackTraceCollector().print_stacktrace();

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;

	while ((pos = message.find('\n', prev)) != std::string::npos)
	{
		TLOG(TLVL_DEBUG) << message.substr(prev, pos - prev);
		prev = pos + 1;
	}

	TLOG(TLVL_DEBUG) << message.substr(prev);
}

BOOST_AUTO_TEST_CASE(PrintStackTrace)
{
	try
	{
		throw int(5);
	}
	catch (int)
	{
		PrintExceptionStackTrace();
	}
}

BOOST_AUTO_TEST_SUITE_END()
