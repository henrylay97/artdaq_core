#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/RawEvent.hh"

#define BOOST_TEST_MODULE(RawEvent_t)
#include <cetlib/quiet_unit_test.hpp>

BOOST_AUTO_TEST_SUITE(RawEvent_test)

BOOST_AUTO_TEST_CASE(RawEventHeader)
{
	artdaq::detail::RawEventHeader reh;

	BOOST_REQUIRE_EQUAL(reh.run_id, 0);
	BOOST_REQUIRE_EQUAL(reh.subrun_id, 0);
	BOOST_REQUIRE_EQUAL(reh.event_id, 0);
	BOOST_REQUIRE_EQUAL(reh.sequence_id, 0);
	BOOST_REQUIRE_EQUAL(reh.timestamp, 0);
	BOOST_REQUIRE_EQUAL(reh.is_complete, false);

	TLOG(TLVL_INFO) << "Default RawEventHeader: " << reh;
}

BOOST_AUTO_TEST_CASE(RawEvent_Methods)
{
	artdaq::RawEvent r1(1, 2, 3, 4, 5);

	BOOST_REQUIRE_EQUAL(r1.wordCount(), 0);
	BOOST_REQUIRE_EQUAL(r1.runID(), 1);
	BOOST_REQUIRE_EQUAL(r1.subrunID(), 2);
	BOOST_REQUIRE_EQUAL(r1.eventID(), 3);
	BOOST_REQUIRE_EQUAL(r1.sequenceID(), 4);
	BOOST_REQUIRE_EQUAL(r1.timestamp(), 5);
	BOOST_REQUIRE_EQUAL(r1.isComplete(), false);

	artdaq::FragmentPtr frag = std::make_unique<artdaq::Fragment>(101, 202, artdaq::Fragment::DataFragmentType, 303);
	r1.insertFragment(std::move(frag));

	r1.markComplete();
	BOOST_REQUIRE_EQUAL(r1.isComplete(), true);

	TLOG(TLVL_INFO) << "RawEvent: " << r1;
}

BOOST_AUTO_TEST_CASE(InsertFragment)
{
	// SCF - The RawEvent::insertFragment() method used to check and verify that
	// the sequence ID of the fragment equaled the sequence ID in the RawEvent
	// header.  This doesn't work for the DS50 aggregator as it packs multiple
	// fragments with different sequence IDs into a single RawEvent.  This test
	// verifies that the we're able to do this.
	artdaq::RawEvent r1(1, 2, 3, 4, 5);
	std::unique_ptr<artdaq::Fragment> f1(new artdaq::Fragment(1, 1));
	std::unique_ptr<artdaq::Fragment> f2(new artdaq::Fragment(2, 1));
	std::unique_ptr<artdaq::Fragment> f3(new artdaq::Fragment(3, 1));

	r1.insertFragment(std::move(f1));
	r1.insertFragment(std::move(f2));
	r1.insertFragment(std::move(f3));
	BOOST_REQUIRE_EQUAL(r1.numFragments(), 3);

	f1.reset(nullptr);
	BOOST_REQUIRE_EXCEPTION(r1.insertFragment(std::move(f1)), cet::exception,
	                        [&](cet::exception e) { return e.category() == "LogicError"; });
}

BOOST_AUTO_TEST_SUITE_END()
