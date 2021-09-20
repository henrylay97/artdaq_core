#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"

#define BOOST_TEST_MODULE configureMessageFacility_t
#include <fhiclcpp/make_ParameterSet.h>
#include <boost/filesystem.hpp>
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "configureMessageFacility_t"
#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(configureMessageFacility_test)

BOOST_AUTO_TEST_CASE(configureTRACETest)
{
	fhicl::ParameterSet pset;
	pset.put("TRACE_MODE", "3");
	fhicl::ParameterSet namlvlset;
	namlvlset.put<std::vector<uint64_t>>("ConfigureMessageFacility_t", {0x0123456789ABCDEF, 0xFEDCBA9876543210, 0x0});
	pset.put("TRACE_NAMLVLSET", namlvlset);
	artdaq::configureTRACE(pset);
}

BOOST_AUTO_TEST_CASE(generateMessageFacilityConfigurationTest)
{
	auto str = artdaq::generateMessageFacilityConfiguration("configureMessageFacility_t");
	std::string badDir = "/this/directory/doesn't/exist";
	setenv("ARTDAQ_LOG_TIMESTAMPS_TO_CONSOLE", "0", 1);
	setenv("ARTDAQ_LOG_ROOT", badDir.c_str(), 1);
	BOOST_REQUIRE_EXCEPTION(artdaq::generateMessageFacilityConfiguration("configureMessageFacility_t"), cet::exception, [](cet::exception const& e) { return e.category() == "ConfigureMessageFacility"; });
	setenv("ARTDAQ_LOG_ROOT", "/tmp", 1);

	setenv("ARTDAQ_LOG_FHICL", "/this/file/doesn't/exist.fcl", 1);
	BOOST_REQUIRE_EXCEPTION(artdaq::generateMessageFacilityConfiguration("configureMessageFacility_t"), cet::exception, [](cet::exception const& e) { return e.category() == "ConfigureMessageFacility"; });
	unsetenv("ARTDAQ_LOG_FHICL");

	artdaq::generateMessageFacilityConfiguration("configureMessageFacility_t");

	boost::filesystem::remove_all("/tmp/configureMessageFacility_t");
}

BOOST_AUTO_TEST_CASE(configureMessageFacilityTest)
{
	setenv("ARTDAQ_LOG_ROOT", "/tmp", 1);
	artdaq::configureMessageFacility("configureMessageFacility_t");
	mf::LogInfo("Test") << "Test Message";
	TLOG(TLVL_INFO) << "Test TRACE";

	boost::filesystem::remove_all("/tmp/configureMessageFacility_t");
}

BOOST_AUTO_TEST_CASE(setMsgFacAppNameTest)
{
	auto appName = artdaq::setMsgFacAppName("test", 1000);
	BOOST_REQUIRE(!appName.empty());
}

BOOST_AUTO_TEST_SUITE_END()