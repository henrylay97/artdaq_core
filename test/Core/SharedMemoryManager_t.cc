#include "artdaq-core/Core/SharedMemoryManager.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "artdaq-core/Utilities/configureMessageFacility.hh"

#define BOOST_TEST_MODULE SharedMemoryManager_t
#include "cetlib/quiet_unit_test.hpp"
#include "cetlib_except/exception.h"

#define TRACE_NAME "SharedMemoryManager_t"
#include "SharedMemoryTestShims.hh"
#include "tracemf.h"

BOOST_AUTO_TEST_SUITE(SharedMemoryManager_test)

BOOST_AUTO_TEST_CASE(Construct)
{
	artdaq::configureMessageFacility("SharedMemoryManager_t", true, true);
	TLOG(TLVL_DEBUG) << "BEGIN TEST Construct";
	uint32_t key = GetRandomKey(0x7357);
	artdaq::SharedMemoryManager man(key, 10, 0x1000, 0x10000);
	BOOST_REQUIRE_EQUAL(man.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man.GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man.size(), 10);
	BOOST_REQUIRE_EQUAL(man.GetAttachedCount(), 1);
	BOOST_REQUIRE_EQUAL(man.GetKey(), key);
	TLOG(TLVL_DEBUG) << "END TEST Construct";
}

BOOST_AUTO_TEST_CASE(Attach)
{
	TLOG(TLVL_DEBUG) << "BEGIN TEST Attach";
	uint32_t key = GetRandomKey(0x7357);
	artdaq::SharedMemoryManager man(key, 10, 0x1000, 0x10000);
	artdaq::SharedMemoryManager man2(key);

	BOOST_REQUIRE_EQUAL(man.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man.GetMyId(), 0);
	BOOST_REQUIRE_EQUAL(man.size(), 10);
	BOOST_REQUIRE_EQUAL(man.GetAttachedCount(), 2);
	BOOST_REQUIRE_EQUAL(man.GetKey(), key);

	BOOST_REQUIRE_EQUAL(man2.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2.GetMyId(), 1);
	BOOST_REQUIRE_EQUAL(man2.size(), 10);
	BOOST_REQUIRE_EQUAL(man2.GetAttachedCount(), 2);
	BOOST_REQUIRE_EQUAL(man2.GetKey(), key);

	TLOG(TLVL_DEBUG) << "END TEST Attach";
}

BOOST_AUTO_TEST_CASE(DataFlow)
{
	TLOG(TLVL_DEBUG) << "BEGIN TEST DataFlow";
	uint32_t key = GetRandomKey(0x7357);
	artdaq::SharedMemoryManager man(key, 10, 0x1000);
	artdaq::SharedMemoryManager man2(key);

	BOOST_REQUIRE_EQUAL(man.ReadyForWrite(false), true);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 10);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man.ReadReadyCount(), 0);

	int buf = man.GetBufferForWriting(false);
	BOOST_REQUIRE_EQUAL(man.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Writing), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0);

	uint8_t n = 0;
	uint8_t data[0x1000];
	std::generate_n(data, 0x1000, [&]() { return ++n; });
	man.Write(buf, data, 0x1000);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0x1000);
	BOOST_REQUIRE_EQUAL(man2.BufferDataSize(buf), 0x1000);
	man.MarkBufferFull(buf, 1);
	BOOST_REQUIRE_EQUAL(man2.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Full), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 1);

	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), true);
	BOOST_REQUIRE_EQUAL(man2.ReadReadyCount(), 1);

	auto readbuf = man2.GetBufferForReading();
	BOOST_REQUIRE_EQUAL(man2.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Reading), true);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	uint8_t byte;
	auto sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);  // ++n means that the first entry will be 1
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 2);  // Second entry is 2
	man2.IncrementReadPos(readbuf, 0x10);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 0x13);  // Read increments, so it would have read 3, but we added 0x10, so we expect 0x13.
	man2.ResetReadPos(readbuf);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);
	man2.IncrementReadPos(readbuf, 0xFFF);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), false);
	man2.MarkBufferEmpty(readbuf);

	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 10);
	TLOG(TLVL_DEBUG) << "END TEST DataFlow";
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
	artdaq::configureMessageFacility("SharedMemoryManager_t", true, true);
	TLOG(TLVL_DEBUG) << "BEGIN TEST Exceptions";
	uint32_t key = GetRandomKey(0x7357);
	artdaq::SharedMemoryManager man(key, 10, 0x1000);
	artdaq::SharedMemoryManager man2(key);
	BOOST_REQUIRE_EQUAL(man.ReadyForWrite(false), true);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 10);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man.ReadReadyCount(), 0);

	// Trying to get an invalid buffer is an exception
	BOOST_REQUIRE_EXCEPTION(man.ResetReadPos(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.BufferDataSize(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.ResetWritePos(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.IncrementReadPos(11, 1), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.IncrementWritePos(11, 1), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.MoreDataInBuffer(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.CheckBuffer(11, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Full), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.MarkBufferFull(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.MarkBufferEmpty(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.ResetBuffer(11), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	int dummy[2]{0, 1};
	BOOST_REQUIRE_EXCEPTION(man.Write(11, &dummy, sizeof(dummy)), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();
	BOOST_REQUIRE_EXCEPTION(man.Read(11, &dummy, sizeof(dummy)), cet::exception, [&](cet::exception e) { return e.category() == "ArgumentOutOfRange"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);
	man.Attach();

	// Trying to access a buffer that is in the wrong state is an exception
	BOOST_REQUIRE_EXCEPTION(man.MarkBufferEmpty(0), cet::exception, [&](cet::exception e) { return e.category() == "StateAccessViolation"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);

	man.Attach();

	// Writing too much data is an exception
	int buf = man.GetBufferForWriting(false);
	BOOST_REQUIRE_EQUAL(man.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Writing), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0);

	uint8_t n = 0;
	uint8_t data[0x2000];
	std::generate_n(data, 0x2000, [&]() { return ++n; });
	BOOST_REQUIRE_EXCEPTION(man.Write(buf, data, 0x2000), cet::exception, [&](cet::exception e) { return e.category() == "SharedMemoryWrite"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);

	man.Attach();
	man2.Attach();
	buf = man.GetBufferForWriting(false);
	BOOST_REQUIRE_EQUAL(man.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Writing), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0);
	man.Write(buf, data, 0x1000);
	man.MarkBufferFull(buf);

	// Reading too much data is an exception
	BOOST_REQUIRE_EQUAL(man2.IsValid(), true);
	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), true);
	BOOST_REQUIRE_EQUAL(man2.ReadReadyCount(), 1);

	int readbuf = man2.GetBufferForReading();
	BOOST_REQUIRE_EQUAL(readbuf, buf);
	BOOST_REQUIRE_EXCEPTION(man2.Read(readbuf, data, 0x1001), cet::exception, [&](cet::exception e) { return e.category() == "SharedMemoryRead"; });
	BOOST_REQUIRE_EQUAL(man2.IsValid(), false);

	man.Attach();
	man2.Attach();

	man.MarkBufferEmpty(readbuf, true);
	buf = man.GetBufferForWriting(false);
	BOOST_REQUIRE_EQUAL(man.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Writing), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0);
	man.Write(buf, data, 0x1000);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0x1000);
	man.MarkBufferFull(buf);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0x1000);

	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), true);
	BOOST_REQUIRE_EQUAL(man2.ReadReadyCount(), 1);
	readbuf = man2.GetBufferForReading();
	BOOST_REQUIRE_EQUAL(readbuf, buf);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager()[0], readbuf);
	BOOST_REQUIRE_EQUAL(man2.BufferDataSize(readbuf), 0x1000);
	// Accessing a buffer that is not owned by the manager is an exception
	BOOST_REQUIRE_EXCEPTION(man.MarkBufferEmpty(readbuf), cet::exception, [&](cet::exception e) { return e.category() == "OwnerAccessViolation"; });
	BOOST_REQUIRE_EQUAL(man.IsValid(), false);

	TLOG(TLVL_DEBUG) << "END TEST Exceptions";
}

BOOST_AUTO_TEST_CASE(Broadcast)
{
	TLOG(TLVL_DEBUG) << "BEGIN TEST Broadcast";
	uint32_t key = GetRandomKey(0x7357);
	artdaq::SharedMemoryManager man(key, 10, 0x1000, 0x10000, false);
	artdaq::SharedMemoryManager man2(key);
	artdaq::SharedMemoryManager man3(key);

	BOOST_REQUIRE_EQUAL(man.ReadyForWrite(false), true);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 10);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man.ReadReadyCount(), 0);

	int buf = man.GetBufferForWriting(false);
	BOOST_REQUIRE_EQUAL(man.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Writing), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager()[0], buf);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0);

	uint8_t n = 0;
	uint8_t data[0x1000];
	std::generate_n(data, 0x1000, [&]() { return ++n; });
	man.Write(buf, data, 0x1000);
	BOOST_REQUIRE_EQUAL(man.BufferDataSize(buf), 0x1000);
	BOOST_REQUIRE_EQUAL(man2.BufferDataSize(buf), 0x1000);
	man.MarkBufferFull(buf, -1);
	BOOST_REQUIRE_EQUAL(man2.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Full), true);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 0);

	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), true);
	BOOST_REQUIRE_EQUAL(man2.ReadReadyCount(), 1);

	auto readbuf = man2.GetBufferForReading();
	BOOST_REQUIRE_EQUAL(readbuf, buf);
	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man3.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man2.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Reading), true);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	uint8_t byte;
	auto sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);  // ++n means that the first entry will be 1
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 2);  // Second entry is 2
	man2.IncrementReadPos(readbuf, 0x10);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 0x13);  // Read increments, so it would have read 3, but we added 0x10, so we expect 0x13.
	man2.ResetReadPos(readbuf);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), true);
	sts = man2.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);
	man2.IncrementReadPos(readbuf, 0xFFF);
	BOOST_REQUIRE_EQUAL(man2.MoreDataInBuffer(readbuf), false);
	man2.MarkBufferEmpty(readbuf);
	BOOST_REQUIRE_EQUAL(man3.ReadyForRead(), true);
	BOOST_REQUIRE_EQUAL(man3.ReadReadyCount(), 1);
	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man2.ReadReadyCount(), 0);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);

	readbuf = man3.GetBufferForReading();
	BOOST_REQUIRE_EQUAL(readbuf, buf);
	BOOST_REQUIRE_EQUAL(man3.GetBuffersOwnedByManager().size(), 1);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man3.CheckBuffer(buf, artdaq::SharedMemoryManager::BufferSemaphoreFlags::Reading), true);
	BOOST_REQUIRE_EQUAL(man3.MoreDataInBuffer(readbuf), true);
	sts = man3.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);  // ++n means that the first entry will be 1
	BOOST_REQUIRE_EQUAL(man3.MoreDataInBuffer(readbuf), true);
	sts = man3.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 2);  // Second entry is 2
	man3.IncrementReadPos(readbuf, 0x10);
	BOOST_REQUIRE_EQUAL(man3.MoreDataInBuffer(readbuf), true);
	sts = man3.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 0x13);  // Read increments, so it would have read 3, but we added 0x10, so we expect 0x13.
	man3.ResetReadPos(readbuf);
	BOOST_REQUIRE_EQUAL(man3.MoreDataInBuffer(readbuf), true);
	sts = man3.Read(readbuf, &byte, 1);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(byte, 1);
	man3.IncrementReadPos(readbuf, 0xFFF);
	BOOST_REQUIRE_EQUAL(man3.MoreDataInBuffer(readbuf), false);
	man3.MarkBufferEmpty(readbuf);
	BOOST_REQUIRE_EQUAL(man3.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man2.ReadyForRead(), false);
	BOOST_REQUIRE_EQUAL(man.ReadyForRead(), false);

	BOOST_REQUIRE_EQUAL(man2.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.GetBuffersOwnedByManager().size(), 0);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 9);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(true), 10);
	sleep(1);
	BOOST_REQUIRE_EQUAL(man.WriteReadyCount(false), 10);
	TLOG(TLVL_DEBUG) << "END TEST Broadcast";
}

BOOST_AUTO_TEST_SUITE_END()
