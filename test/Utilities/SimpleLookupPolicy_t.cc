#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"

#define BOOST_TEST_MODULE SimpleLookupPolicy_t
#include <boost/filesystem.hpp>
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "SimpleLookupPolicy_t"
#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(SimpleLookupPolicy_test)

BOOST_AUTO_TEST_CASE(Constructors)
{
	artdaq::SimpleLookupPolicy e("PATH");
	artdaq::SimpleLookupPolicy np("", artdaq::SimpleLookupPolicy::ArgType::PATH_STRING);
	artdaq::SimpleLookupPolicy p("/tmp", artdaq::SimpleLookupPolicy::ArgType::PATH_STRING);
	BOOST_REQUIRE(true);  // No exceptions
}

BOOST_AUTO_TEST_CASE(AbsoluteFilePath)
{
	artdaq::SimpleLookupPolicy p("", artdaq::SimpleLookupPolicy::ArgType::PATH_STRING);
	auto absolutePath = boost::filesystem::current_path();
	absolutePath += "LookupTarget.fcl";
	p(absolutePath.string());
	BOOST_REQUIRE(true);  // No exceptions
}

BOOST_AUTO_TEST_CASE(FallbackPath)
{
	auto coreDir = getenv("ARTDAQ_CORE_DIR");
	std::string coreDirStr = "";
	if (coreDir != nullptr)
	{
		coreDirStr = std::string(coreDir);
	}

	artdaq::SimpleLookupPolicy p("/tmp:.:" + coreDirStr + "/test/Utilities/fcl", artdaq::SimpleLookupPolicy::ArgType::PATH_STRING);
	p("LookupTarget.fcl");
	BOOST_REQUIRE_EXCEPTION(p("ThisFileDoesNotExist.fcl"), cet::exception, [](cet::exception const& e) { return e.category() == "search_path"; });
}

BOOST_AUTO_TEST_SUITE_END()