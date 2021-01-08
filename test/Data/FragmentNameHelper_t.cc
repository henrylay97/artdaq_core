#include "artdaq-core/Data/ContainerFragmentLoader.hh"
#include "artdaq-core/Data/FragmentNameHelper.hh"

#define BOOST_TEST_MODULE(FragmentNameHelper_t)
#include <cetlib/quiet_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FragmentNameHelper_test)

BOOST_AUTO_TEST_CASE(FNH_Construct)
{
	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");
}

BOOST_AUTO_TEST_CASE(FNH_ExtraTypesInConstructor)
{
	auto extraType = std::make_pair(artdaq::Fragment::FirstUserFragmentType, "Test");

	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {extraType});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::FirstUserFragmentType), "Test");
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::FirstUserFragmentType + 2), "testunidentified");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");
}

BOOST_AUTO_TEST_CASE(FNH_ExtraTypesOverwriteInConstructor)
{
	auto extraType = std::make_pair(artdaq::Fragment::DataFragmentType, "Test");

	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {extraType});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Test");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");
}

BOOST_AUTO_TEST_CASE(FNH_ExtraTypesMethod)
{
	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");

	helper->AddExtraType(artdaq::Fragment::FirstUserFragmentType, "Test");
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::FirstUserFragmentType), "Test");
}

BOOST_AUTO_TEST_CASE(FNH_ExtraTypesOverwriteMethod)
{
	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");

	helper->AddExtraType(artdaq::Fragment::DataFragmentType, "Test");
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Test");
}

BOOST_AUTO_TEST_CASE(FNH_DecodeFragment)
{
	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");

	artdaq::Fragment frag(1, 2, artdaq::Fragment::DataFragmentType, 3);

	auto res = helper->GetInstanceNameForFragment(frag);
	BOOST_REQUIRE_EQUAL(res.first, true);
	BOOST_REQUIRE_EQUAL(res.second, "Data");

	frag.setUserType(artdaq::Fragment::FirstUserFragmentType + 2);
	res = helper->GetInstanceNameForFragment(frag);
	BOOST_REQUIRE_EQUAL(res.first, false);
	BOOST_REQUIRE_EQUAL(res.second, "testunidentified");
}

BOOST_AUTO_TEST_CASE(FNH_DecodeContainerFragment)
{
	auto helper = artdaq::makeNameHelper("Artdaq", "testunidentified", {});
	auto names = helper->GetAllProductInstanceNames();
	BOOST_REQUIRE(names.size() > 0);
	BOOST_REQUIRE_EQUAL(helper->GetInstanceNameForType(artdaq::Fragment::DataFragmentType), "Data");

	BOOST_REQUIRE_EQUAL(helper->GetUnidentifiedInstanceName(), "testunidentified");

	artdaq::Fragment frag;
	artdaq::ContainerFragmentLoader cfl(frag, artdaq::Fragment::DataFragmentType);
	auto res = helper->GetInstanceNameForFragment(frag);
	BOOST_REQUIRE_EQUAL(res.first, true);
	BOOST_REQUIRE_EQUAL(res.second, "ContainerData");
}

BOOST_AUTO_TEST_SUITE_END()
