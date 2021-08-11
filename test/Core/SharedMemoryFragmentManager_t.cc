#define TRACE_NAME "SharedMemoryFragmentManager_t"

#include <memory>

#include "artdaq-core/Core/SharedMemoryFragmentManager.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "tracemf.h"

#define BOOST_TEST_MODULE(SharedMemoryFragmentManager_t)
#include "SharedMemoryTestShims.hh"
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

BOOST_AUTO_TEST_SUITE(SharedMemoryFragmentManager_test)

BOOST_AUTO_TEST_CASE(Construct)
{
	artdaq::configureMessageFacility("SharedMemoryFragmentManager_t", true, true);
	TLOG(TLVL_INFO) << "BEGIN TEST Construct";
	artdaq::SharedMemoryFragmentManager man(GetRandomKey(0xF4A6), 10, 0x1000);
	BOOST_REQUIRE_EQUAL(man.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man.GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man.size(), 10);
	BOOST_REQUIRE_EQUAL(man.GetAttachedCount(), 1);
	TLOG(TLVL_INFO) << "END TEST Construct";
}

BOOST_AUTO_TEST_CASE(Attach)
{
	TLOG(TLVL_INFO) << "BEGIN TEST Attach";
	uint32_t key = GetRandomKey(0xF4A6);
	artdaq::SharedMemoryFragmentManager man(key, 10, 0x1000);
	artdaq::SharedMemoryFragmentManager man2(key);

	BOOST_REQUIRE_EQUAL(man.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man.GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man.size(), 10);
	BOOST_REQUIRE_EQUAL(man.GetAttachedCount(), 2);

	BOOST_REQUIRE_EQUAL(man2.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2.GetMyId(), 1);
	BOOST_REQUIRE_EQUAL(man2.size(), 10);
	BOOST_REQUIRE_EQUAL(man2.GetAttachedCount(), 2);
	TLOG(TLVL_INFO) << "END TEST Attach";
}

BOOST_AUTO_TEST_CASE(Reattach)
{
	TLOG(TLVL_INFO) << "BEGIN TEST Reattach";
	uint32_t key = GetRandomKey(0xF4A6);
	std::unique_ptr<artdaq::SharedMemoryFragmentManager> man(new artdaq::SharedMemoryFragmentManager(key, 10, 0x1000));
	std::unique_ptr<artdaq::SharedMemoryFragmentManager> man2(new artdaq::SharedMemoryFragmentManager(key));

	BOOST_REQUIRE_EQUAL(man->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man->GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man->size(), 10);
	BOOST_REQUIRE_EQUAL(man->GetAttachedCount(), 2);

	BOOST_REQUIRE_EQUAL(man2->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2->GetMyId(), 1);
	BOOST_REQUIRE_EQUAL(man2->size(), 10);
	BOOST_REQUIRE_EQUAL(man2->GetAttachedCount(), 2);

	man2.reset(nullptr);
	BOOST_REQUIRE_EQUAL(man->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man->GetAttachedCount(), 1);

	man2 = std::make_unique<artdaq::SharedMemoryFragmentManager>(key);
	BOOST_REQUIRE_EQUAL(man->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man->GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man->size(), 10);
	BOOST_REQUIRE_EQUAL(man->GetAttachedCount(), 2);

	BOOST_REQUIRE_EQUAL(man2->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2->GetMyId(), 2);
	BOOST_REQUIRE_EQUAL(man2->size(), 10);
	BOOST_REQUIRE_EQUAL(man2->GetAttachedCount(), 2);

	man.reset(nullptr);
	BOOST_REQUIRE_EQUAL(man2->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2->IsEndOfData(), true);
	BOOST_REQUIRE_EQUAL(man2->GetMyId(), 2);
	BOOST_REQUIRE_EQUAL(man2->size(), 10);
	BOOST_REQUIRE_EQUAL(man2->GetAttachedCount(), 1);

	man2->Attach();
	BOOST_REQUIRE_EQUAL(man2->IsValid(), false);

	man = std::make_unique<artdaq::SharedMemoryFragmentManager>(key, 10, 0x1000);
	BOOST_REQUIRE_EQUAL(man->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man->GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man->size(), 10);
	BOOST_REQUIRE_EQUAL(man->GetAttachedCount(), 1);

	man2->Attach();
	BOOST_REQUIRE_EQUAL(man2->IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2->GetMyId(), 1);
	BOOST_REQUIRE_EQUAL(man2->size(), 10);
	BOOST_REQUIRE_EQUAL(man->GetAttachedCount(), 2);
	BOOST_REQUIRE_EQUAL(man2->GetAttachedCount(), 2);

	TLOG(TLVL_INFO) << "END TEST Reattach";
}

BOOST_AUTO_TEST_CASE(DataFlow)
{
	TLOG(TLVL_INFO) << "BEGIN TEST DataFlow";
	TLOG(TLVL_DEBUG) << "Initializing SharedMemoryFragmentManagers for DataFlow test";
	uint32_t key = GetRandomKey(0xF4A6);
	artdaq::SharedMemoryFragmentManager man(key, 10, 0x1000);
	artdaq::SharedMemoryFragmentManager man2(key);

	auto fragSizeWords = 0x1000 / sizeof(artdaq::RawDataType) - artdaq::detail::RawFragmentHeader::num_words() - 1;

	TLOG(TLVL_DEBUG) << "Creating test Fragment";
	artdaq::Fragment frag(fragSizeWords);
	frag.setSequenceID(0x10);
	frag.setFragmentID(0x20);
	auto type = artdaq::Fragment::DataFragmentType;
	frag.setSystemType(type);
	frag.setTimestamp(0x30);
	for (size_t ii = 0; ii < fragSizeWords; ++ii)
	{
		*(frag.dataBegin() + ii) = ii;
	}

	TLOG(TLVL_DEBUG) << "Writing Test Fragment to Shared Memory";
	auto fragSize = frag.size();
	man.WriteFragment(std::move(frag), false, 0);

	TLOG(TLVL_DEBUG) << "Reading Test Fragment Header";
	artdaq::detail::RawFragmentHeader header;
	auto sts = man2.ReadFragmentHeader(header);

	TLOG(TLVL_DEBUG) << "Checking Test Fragment Header Contents";
	BOOST_REQUIRE_EQUAL(sts, 0);
	BOOST_REQUIRE_EQUAL(static_cast<size_t>(header.word_count), fragSize);
	BOOST_REQUIRE_EQUAL(static_cast<size_t>(header.sequence_id), 0x10);
	BOOST_REQUIRE_EQUAL(static_cast<size_t>(header.fragment_id), 0x20);
	BOOST_REQUIRE_EQUAL(static_cast<size_t>(header.type), type);
	BOOST_REQUIRE_EQUAL(static_cast<size_t>(header.timestamp), 0x30);

	TLOG(TLVL_DEBUG) << "Reading Test Fragment data";
	artdaq::Fragment frag2(header.word_count);
	sts = man2.ReadFragmentData(frag2.dataBegin(), header.word_count - header.num_words());

	TLOG(TLVL_DEBUG) << "Checking Test Fragment contents";
	BOOST_REQUIRE_EQUAL(sts, 0);
	for (size_t ii = 0; ii < fragSizeWords; ++ii)
	{
		BOOST_REQUIRE_EQUAL(ii, *(frag2.dataBegin() + ii));
	}
	TLOG(TLVL_DEBUG) << "SharedMemoryFragmentManager DataFlow test complete";
	TLOG(TLVL_INFO) << "END TEST DataFlow";
}

BOOST_AUTO_TEST_CASE(WholeFragment)
{
	TLOG(TLVL_INFO) << "BEGIN TEST WholeFragment";
	TLOG(TLVL_DEBUG) << "Initializing SharedMemoryFragmentManagers for WholeFragment Test";
	uint32_t key = GetRandomKey(0xF4A6);
	artdaq::SharedMemoryFragmentManager man(key, 10, 0x1000);
	artdaq::SharedMemoryFragmentManager man2(key);

	auto fragSizeWords = 0x1000 / sizeof(artdaq::RawDataType) - artdaq::detail::RawFragmentHeader::num_words() - 1;

	TLOG(TLVL_DEBUG) << "Creating test Fragment";
	artdaq::Fragment frag(fragSizeWords);
	frag.setSequenceID(0x10);
	frag.setFragmentID(0x20);
	auto type = artdaq::Fragment::DataFragmentType;
	frag.setSystemType(type);
	frag.setTimestamp(0x30);
	for (size_t ii = 0; ii < fragSizeWords; ++ii)
	{
		*(frag.dataBegin() + ii) = ii;
	}

	TLOG(TLVL_DEBUG) << "Writing Test Fragment to Shared Memory";
	auto fragSize = frag.size();
	man.WriteFragment(std::move(frag), false, 0);

	TLOG(TLVL_DEBUG) << "Reading Test Fragment Header";
	artdaq::Fragment recvdFrag;
	auto sts = man2.ReadFragment(recvdFrag);

	TLOG(TLVL_DEBUG) << "Checking Test Fragment Header Contents";
	BOOST_REQUIRE_EQUAL(sts, 0);
	BOOST_REQUIRE_EQUAL(recvdFrag.size(), fragSize);
	BOOST_REQUIRE_EQUAL(recvdFrag.sequenceID(), 0x10);
	BOOST_REQUIRE_EQUAL(recvdFrag.fragmentID(), 0x20);
	BOOST_REQUIRE_EQUAL(recvdFrag.type(), type);
	BOOST_REQUIRE_EQUAL(recvdFrag.timestamp(), 0x30);

	TLOG(TLVL_DEBUG) << "Checking Test Fragment Data Contents";
	for (size_t ii = 0; ii < fragSizeWords; ++ii)
	{
		//TLOG(TLVL_DEBUG) << *(frag.dataBegin() + ii) << " =?= " << *(recvdFrag.dataBegin() + ii) ;
		BOOST_REQUIRE_EQUAL(ii, *(recvdFrag.dataBegin() + ii));
	}
	TLOG(TLVL_DEBUG) << "SharedMemoryFragmentManager WholeFragment test complete";
	TLOG(TLVL_INFO) << "END TEST WholeFragment";
}

BOOST_AUTO_TEST_CASE(Timeout)
{
	TLOG(TLVL_INFO) << "BEGIN TEST Timeout";
	TLOG(TLVL_DEBUG) << "Initializing SharedMemoryFragmentManagers for Timeout Test";
	uint32_t key = GetRandomKey(0xF4A6);
	artdaq::SharedMemoryFragmentManager man(key, 1, 0x1000);

	auto fragSizeWords = 0x1000 / sizeof(artdaq::RawDataType) - artdaq::detail::RawFragmentHeader::num_words() - 1;

	TLOG(TLVL_DEBUG) << "Creating test Fragment";
	artdaq::Fragment frag(fragSizeWords);
	frag.setSequenceID(0x10);
	frag.setFragmentID(0x20);
	auto type = artdaq::Fragment::DataFragmentType;
	frag.setSystemType(type);
	frag.setTimestamp(0x30);
	for (size_t ii = 0; ii < fragSizeWords; ++ii)
	{
		*(frag.dataBegin() + ii) = ii;
	}

	TLOG(TLVL_DEBUG) << "Reserving buffer to cause timeout to happen";
	auto ret = man.GetBufferForWriting(true);
	BOOST_REQUIRE_EQUAL(ret, 0);

	TLOG(TLVL_DEBUG) << "Attempting to write Fragment to Shared Memory. This should time out.";
	auto start_time = std::chrono::steady_clock::now();
	ret = man.WriteFragment(std::move(frag), true, 100000);
	auto duration = artdaq::TimeUtils::GetElapsedTimeMicroseconds(start_time);

	BOOST_REQUIRE_EQUAL(ret, -3);
	BOOST_REQUIRE_GE(duration, 100000);

	TLOG(TLVL_DEBUG) << "SharedMemoryFragmentManager Timeout test complete";
	TLOG(TLVL_INFO) << "END TEST Timeout";
}

BOOST_AUTO_TEST_SUITE_END()
