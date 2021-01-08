#include "artdaq-core/Data/PackageBuildInfo.hh"

#define BOOST_TEST_MODULE(BuildInfo_t)
#include <cetlib/quiet_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(BuildInfo_test)

BOOST_AUTO_TEST_CASE(BuildInfo_Methods)
{
	artdaq::PackageBuildInfo nfo;

	BOOST_REQUIRE_EQUAL(nfo.getBuildTimestamp(), "");
	BOOST_REQUIRE_EQUAL(nfo.getPackageName(), "");
	BOOST_REQUIRE_EQUAL(nfo.getPackageVersion(), "");

	nfo.setBuildTimestamp("now");
	nfo.setPackageName("Test");
	nfo.setPackageVersion("v1.0");

	BOOST_REQUIRE_EQUAL(nfo.getBuildTimestamp(), "now");
	BOOST_REQUIRE_EQUAL(nfo.getPackageName(), "Test");
	BOOST_REQUIRE_EQUAL(nfo.getPackageVersion(), "v1.0");
}

BOOST_AUTO_TEST_SUITE_END()
