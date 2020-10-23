#define TRACE_NAME "SimpleMemoryReader"
#include "artdaq-core/Core/SimpleMemoryReader.hh"

#include <chrono>   // for milliseconds
#include <cstddef>  // for std::size_t
#include <iostream>
#include <string>
#include "tracemf.h"  // TRACE

namespace artdaq {
int SimpleMemoryReaderApp(int argc, char** argv)
{
	try
	{
		size_t eec(0);
		if (argc == 2)
		{
			std::istringstream ins(argv[1]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			ins >> eec;
		}
		SimpleMemoryReader reader(0xA99, 0xB99, eec);
		reader.run();
		return 0;
	}
	catch (std::string const& msg)
	{
		std::cerr << "SimpleMemoryReaderApp failed: " << msg;
		return 1;
	}
	catch (...)
	{
		return 1;
	}
}

SimpleMemoryReader::
    SimpleMemoryReader(uint32_t shm_key, uint32_t broadcast_key, std::size_t expectedEventCount)
    : incoming_events_(new SharedMemoryEventReceiver(shm_key, broadcast_key))
    , expectedEventCount_(expectedEventCount)
{
	TLOG(50) << "ctor done (after queue_.setReaderIsReady())";
}

void SimpleMemoryReader::run()
{
	std::size_t eventsSeen = 0;
	auto doPrint = getenv("VERBOSE_QUEUE_READING");
	while (true)
	{
		bool keep_looping = true;
		bool got_event = false;
		while (keep_looping)
		{
			keep_looping = false;
			got_event = incoming_events_->ReadyForRead();
			if (!got_event)
			{
				TLOG(TLVL_INFO) << "InputFailure: Reading timed out in SharedMemoryReader::readNext()";
				keep_looping = true;
			}
		}

		if (!got_event)
		{
			break;
		}

		auto errflag = false;
		auto evtHeader = incoming_events_->ReadHeader(errflag);
		if (errflag)
		{
			break;  // Buffer was changed out from under reader!
		}
		auto fragmentTypes = incoming_events_->GetFragmentTypes(errflag);
		if (errflag)
		{
			break;  // Buffer was changed out from under reader!
		}
		if (fragmentTypes.empty())
		{
			TLOG(TLVL_ERROR) << "Event has no Fragments! Aborting!";
			incoming_events_->ReleaseBuffer();
			break;
		}
		auto firstFragmentType = *fragmentTypes.begin();

		// We return false, indicating we're done reading, if:
		//   1) we did not obtain an event, because we timed out and were
		//      configured NOT to keep trying after a timeout, or
		//   2) the event we read was the end-of-data marker: a null
		//      pointer
		if (!got_event || firstFragmentType == Fragment::EndOfDataFragmentType)
		{
			TLOG(TLVL_DEBUG) << "Received shutdown message, returning false";
			incoming_events_->ReleaseBuffer();
			break;
		}

		++eventsSeen;
		RawEvent evt = RawEvent(*evtHeader);
		if (doPrint != nullptr) { std::cout << evt << std::endl; }
		incoming_events_->ReleaseBuffer();
	}
	if ((expectedEventCount_ != 0u) && eventsSeen != expectedEventCount_)
	{
		throw cet::exception("SimpleMemoryReader")  // NOLINT(cert-err60-cpp)
		    << "Wrong number of events in SimpleMemoryReader ("
		    << eventsSeen << " != " << expectedEventCount_ << ").";
	}
}
}  // namespace artdaq
