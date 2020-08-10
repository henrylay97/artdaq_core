#ifndef artdaq_core_Core_SimpleQueueReader_hh
#define artdaq_core_Core_SimpleQueueReader_hh

#include <memory>
#include "artdaq-core/Core/GlobalQueue.hh"

namespace artdaq {
/**
	* \brief An application which pops items off a RawEventQueue using the SimpleQueueReader
	* \param argc Number of arguments (OS-provided)
	* \param argv Array of argument strings (OS-provided)
	* \return Status code: 0 success, 1 for any error
	*
	* simpleQueueReaderApp is a function that can be used in place of
	* artapp(), to read RawEvent objects from the shared RawEvent queue.
	* Note that it ignores both of its arguments.
	*/
int simpleQueueReaderApp(int argc, char** argv);

/**
   * \brief SimpleQueueReader will continue to read RawEvent objects off the queue
   * until it encounters a null pointer, at which point it stops.
   */
class SimpleQueueReader
{
public:
	/**
		 * \brief Constructs a SimpleQueueReader
		 * \param expectedEventCount The number of events the SimpleQueueReader should expect
		 */
	explicit SimpleQueueReader(std::size_t eec = 0);

	/**
		 * \brief Run until a null pointer is popped off of the RawEventQueue. Throws an excpetion
		 * if expectedEventCount is set and a different number of events come off the queue.
		 * \exception std::string When the expectedEventCount is set and a different number of events come off the queue.
		 */
	void run();

private:
	/**
		 * \brief Reference to the queue of RawEvent_ptr objects
		 */
	RawEventQueue& queue_;
	/**
		 * \brief For testing purposes, the expected number of events
		 */
	std::size_t expectedEventCount_;
};
}  // namespace artdaq

#endif /* artdaq_core_Core_SimpleQueueReader_hh */
