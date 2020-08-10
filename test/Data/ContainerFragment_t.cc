#include "artdaq-core/Data/ContainerFragmentLoader.hh"

#define BOOST_TEST_MODULE(ContainerFragment_t)
#include "cetlib/quiet_unit_test.hpp"

BOOST_AUTO_TEST_SUITE(ContainerFragment_test)

BOOST_AUTO_TEST_CASE(Construct)
{
	artdaq::Fragment f(0);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	BOOST_REQUIRE_EQUAL(f.dataSize(), 1);
	BOOST_REQUIRE_EQUAL(cf->block_count(), 0);
	auto type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf->fragment_type(), type);
	BOOST_REQUIRE_EQUAL(*reinterpret_cast<const size_t*>(cf->dataBegin()), artdaq::ContainerFragment::CONTAINER_MAGIC);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

BOOST_AUTO_TEST_CASE(AddEmptyFragment)
{
	auto frag = new artdaq::Fragment();
	frag->setSequenceID(0);
	frag->setSystemType(artdaq::Fragment::EmptyFragmentType);

	artdaq::Fragment f(0);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	cfl.addFragment(*frag);

	BOOST_REQUIRE_EQUAL(f.dataSizeBytes(), sizeof(artdaq::detail::RawFragmentHeader) + (2 * sizeof(size_t)));
	BOOST_REQUIRE_EQUAL(cf->block_count(), 1);
	auto type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf->fragment_type(), type);

	delete frag;
}

BOOST_AUTO_TEST_CASE(AddFragment_Ptr)
{
	std::vector<artdaq::Fragment::value_type> fakeData{1, 2, 3, 4};
	artdaq::FragmentPtr
	    tmpFrag(artdaq::Fragment::dataFrag(1,
	                                       0,
	                                       fakeData.begin(),
	                                       fakeData.end()));
	tmpFrag->setUserType(artdaq::Fragment::FirstUserFragmentType);

	artdaq::Fragment f(0);
	f.setSequenceID(1);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	cfl.addFragment(tmpFrag);

	BOOST_REQUIRE_EQUAL(f.dataSizeBytes(), sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + (2 * sizeof(size_t)));
	BOOST_REQUIRE_EQUAL(f.sizeBytes(), 2 * sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + (2 * sizeof(size_t)) + sizeof(artdaq::ContainerFragment::Metadata));
	BOOST_REQUIRE_EQUAL(cf->block_count(), 1);
	auto type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf->fragment_type(), type);
	BOOST_REQUIRE_EQUAL(cf->fragSize(0), tmpFrag->sizeBytes());

	auto outfrag = cf->at(0);
	BOOST_REQUIRE_EQUAL(outfrag->sequenceID(), 1);
	BOOST_REQUIRE_EQUAL(outfrag->fragmentID(), 0);
	BOOST_REQUIRE_EQUAL(outfrag->dataSize(), 4);
	BOOST_REQUIRE_EQUAL(*outfrag->dataBegin(), 1);
	BOOST_REQUIRE_EQUAL(*(outfrag->dataBegin() + 1), 2);
}

BOOST_AUTO_TEST_CASE(AddFragment_Ref)
{
	std::vector<artdaq::Fragment::value_type> fakeData{1, 2, 3, 4};
	artdaq::FragmentPtr
	    tmpFrag(artdaq::Fragment::dataFrag(1,
	                                       0,
	                                       fakeData.begin(),
	                                       fakeData.end()));
	tmpFrag->setUserType(artdaq::Fragment::FirstUserFragmentType);
	auto frag = *tmpFrag.get();

	artdaq::Fragment f(0);
	f.setSequenceID(1);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	cfl.addFragment(frag);

	BOOST_REQUIRE_EQUAL(f.dataSizeBytes(), sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + (2 * sizeof(size_t)));
	BOOST_REQUIRE_EQUAL(f.sizeBytes(), 2 * sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + (2 * sizeof(size_t)) + sizeof(artdaq::ContainerFragment::Metadata));
	BOOST_REQUIRE_EQUAL(cf->block_count(), 1);
	auto type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf->fragment_type(), type);
	BOOST_REQUIRE_EQUAL(cf->fragSize(0), tmpFrag->sizeBytes());

	auto outfrag = cf->at(0);
	BOOST_REQUIRE_EQUAL(outfrag->sequenceID(), 1);
	BOOST_REQUIRE_EQUAL(outfrag->fragmentID(), 0);
	BOOST_REQUIRE_EQUAL(outfrag->dataSize(), 4);
	BOOST_REQUIRE_EQUAL(*outfrag->dataBegin(), 1);
	BOOST_REQUIRE_EQUAL(*(outfrag->dataBegin() + 1), 2);
}

BOOST_AUTO_TEST_CASE(AddFragments)
{
	std::vector<artdaq::Fragment::value_type> fakeData1{1, 2, 3, 4};
	std::vector<artdaq::Fragment::value_type> fakeData2{5, 6, 7, 8};
	artdaq::FragmentPtr
	    tmpFrag1(artdaq::Fragment::dataFrag(1,
	                                        0,
	                                        fakeData1.begin(),
	                                        fakeData1.end()));
	tmpFrag1->setUserType(artdaq::Fragment::FirstUserFragmentType);
	artdaq::FragmentPtr
	    tmpFrag2(artdaq::Fragment::dataFrag(1,
	                                        1,
	                                        fakeData2.begin(),
	                                        fakeData2.end()));
	tmpFrag2->setUserType(artdaq::Fragment::FirstUserFragmentType);
	artdaq::FragmentPtrs frags;
	frags.push_back(std::move(tmpFrag1));
	frags.push_back(std::move(tmpFrag2));

	artdaq::Fragment f(0);
	f.setSequenceID(1);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	cfl.addFragments(frags);

	BOOST_REQUIRE_EQUAL(f.dataSizeBytes(), 2 * (sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + sizeof(size_t)) + sizeof(size_t));
	BOOST_REQUIRE_EQUAL(f.sizeBytes(), 2 * (sizeof(artdaq::detail::RawFragmentHeader) + 4 * sizeof(artdaq::Fragment::value_type) + sizeof(size_t)) + sizeof(size_t) + sizeof(artdaq::detail::RawFragmentHeader) + sizeof(artdaq::ContainerFragment::Metadata));
	BOOST_REQUIRE_EQUAL(cf->block_count(), 2);
	auto type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf->fragment_type(), type);

	auto outfrag = cf->at(0);
	BOOST_REQUIRE_EQUAL(outfrag->sequenceID(), 1);
	BOOST_REQUIRE_EQUAL(outfrag->fragmentID(), 0);
	BOOST_REQUIRE_EQUAL(outfrag->dataSize(), 4);
	BOOST_REQUIRE_EQUAL(*outfrag->dataBegin(), 1);
	BOOST_REQUIRE_EQUAL(*(outfrag->dataBegin() + 1), 2);
	outfrag = cf->at(1);
	BOOST_REQUIRE_EQUAL(outfrag->sequenceID(), 1);
	BOOST_REQUIRE_EQUAL(outfrag->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(outfrag->dataSize(), 4);
	BOOST_REQUIRE_EQUAL(*outfrag->dataBegin(), 5);
	BOOST_REQUIRE_EQUAL(*(outfrag->dataBegin() + 1), 6);
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
	artdaq::Fragment f(0);
	f.setSequenceID(1);
	artdaq::ContainerFragmentLoader cfl(f);
	auto cf = reinterpret_cast<artdaq::ContainerFragment*>(&cfl);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

	// Attempting to access a fragment which is not in the container is an exception
	BOOST_REQUIRE_EXCEPTION(cf->at(0), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EXCEPTION(cf->fragSize(0), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EXCEPTION(cf->fragmentIndex(1), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });

	// Using an already-initialized Fragment to init a Container Fragment is an exception
	artdaq::Fragment f2(2);
	f2.setSequenceID(2);
	f2.setFragmentID(0);
	f2.setUserType(artdaq::Fragment::FirstUserFragmentType);
	BOOST_REQUIRE_EXCEPTION(artdaq::ContainerFragmentLoader cfl2(f2), cet::exception, [&](cet::exception e) { return e.category() == "InvalidFragment"; });

	//// Adding a Fragment to a full Container is an exception
	//for (int ii = 0; ii < artdaq::CONTAINER_FRAGMENT_COUNT_MAX; ++ii)
	//{
	//	cfl.addFragment(f2);
	//}
	//BOOST_REQUIRE_EXCEPTION(cfl.addFragment(f2), cet::exception, [&](cet::exception e) { return e.category() == "ContainerFull"; });

	// Adding a Fragment of different type to a Container is an exception
	artdaq::Fragment f3(0);
	f3.setSequenceID(1);
	artdaq::ContainerFragmentLoader cfl3(f3);
	cfl3.addFragment(f2);
	f2.setSystemType(artdaq::Fragment::EmptyFragmentType);
	BOOST_REQUIRE_EXCEPTION(cfl3.addFragment(f2), cet::exception, [&](cet::exception e) { return e.category() == "WrongFragmentType"; });
}

BOOST_AUTO_TEST_SUITE_END()
