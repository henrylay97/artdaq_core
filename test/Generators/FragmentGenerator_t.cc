#define BOOST_TEST_MODULE (FragmentGenerator_t)
#include <cetlib/quiet_unit_test.hpp>

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Generators/FragmentGenerator.hh"

namespace artdaqtest {
class FragmentGeneratorTest;
}

/**
 * \brief Tests the functionality of the artdaq::FragmentGenerator class
 */
class artdaqtest::FragmentGeneratorTest : public artdaq::FragmentGenerator
{
public:
	FragmentGeneratorTest() = default;

	bool getNext(artdaq::FragmentPtrs& output) override
	{
		return getNext_(output);
	}

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs() override
	{
		return fragmentIDs_();
	}

private:
	bool getNext_(artdaq::FragmentPtrs& /*frags*/);

	std::vector<artdaq::Fragment::fragment_id_t> fragmentIDs_();
};

bool artdaqtest::FragmentGeneratorTest::getNext_(artdaq::FragmentPtrs& frags)
{
	frags.emplace_back(new artdaq::Fragment);
	return true;
}

std::vector<artdaq::Fragment::fragment_id_t>
artdaqtest::FragmentGeneratorTest::
    fragmentIDs_()
{
	return {1};
}

BOOST_AUTO_TEST_SUITE(FragmentGenerator_t)

BOOST_AUTO_TEST_CASE(Simple)
{
	artdaqtest::FragmentGeneratorTest testGen;
	artdaq::FragmentGenerator& baseGen(testGen);
	artdaq::FragmentPtrs fps;
	baseGen.getNext(fps);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
