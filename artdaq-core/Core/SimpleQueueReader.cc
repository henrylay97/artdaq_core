#include "artdaq-core/Core/SimpleQueueReader.hh"

#include <chrono>   // for milliseconds
#include <cstddef>  // for std::size_t
#include <iostream>
#include <string>
#include <thread>   // std::this_thread::sleep_for
#include "trace.h"  // TRACE

namespace artdaq {
int simpleQueueReaderApp(int argc, char** argv)
{
	try
	{
		size_t eec(0);
		if (argc == 2)
		{
			std::istringstream ins(argv[1]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			ins >> eec;
		}
		SimpleQueueReader reader(eec);
		reader.run();
		return 0;
	}
	catch (std::string const& msg)
	{
		std::cerr << "simpleQueueReaderApp failed: "
		          << msg;
		return 1;
	}
	catch (...)
	{
		return 1;
	}
}

SimpleQueueReader::
    SimpleQueueReader(std::size_t expectedEventCount)
    : queue_(getGlobalQueue())
    , expectedEventCount_(expectedEventCount)
{
	queue_.setReaderIsReady();
	TLOG(50) << "SimpleQueueReader ctor done (after queue_.setReaderIsReady())";
}

void SimpleQueueReader::run()
{
	std::size_t eventsSeen = 0;
	auto doPrint = getenv("VERBOSE_QUEUE_READING");
	while (true)
	{
		RawEvent_ptr rawEventPtr;
		if (queue_.deqNowait(rawEventPtr))
		{
			// If we got a null pointer, we're done...
			if (!rawEventPtr) { break; }
			++eventsSeen;
			// Otherwise, do our work ...
			if (doPrint != nullptr) { std::cout << *rawEventPtr << std::endl; }
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
		}
	}
	if ((expectedEventCount_ != 0u) && eventsSeen != expectedEventCount_)
	{
		throw cet::exception("SimpleQueueReader")  // NOLINT(cert-err60-cpp)
		    << "Wrong number of events in SimpleQueueReader ("
		    << eventsSeen << " != " << expectedEventCount_ << ").\n";
	}
}
}  // namespace artdaq
