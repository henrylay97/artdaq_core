#ifndef artdaq_core_Core_SimpleMemoryReader_hh
#define artdaq_core_Core_SimpleMemoryReader_hh 1

#include <memory>
#include "artdaq-core/Core/SharedMemoryEventReceiver.hh"

namespace artdaq {
/**
	* \brief An application which pops items off a RawEventQueue using the SimpleMemoryReader
	* \param argc Number of arguments (OS-provided)
	* \param argv Array of argument strings (OS-provided)
	* \return Status code: 0 success, 1 for any error
	*
	* SimpleMemoryReaderApp is a function that can be used in place of
	* artapp(), to read RawEvent objects from the shared RawEvent queue.
	* Note that it ignores both of its arguments.
	*/
int SimpleMemoryReaderApp(int argc, char** argv);

/**
   * \brief SimpleMemoryReader will continue to read RawEvent objects off the queue
   * until it encounters a null pointer, at which point it stops.
   */
class SimpleMemoryReader
{
public:
	/**
		 * \brief Constructs a SimpleMemoryReader
		 * \param shm_key Key of the shared memory segment
		 * \param broadcast_key Key of the broadcast shared memory segment
		 * \param expectedEventCount The number of events the SimpleMemoryReader should expect
		 */
	explicit SimpleMemoryReader(uint32_t shm_key, uint32_t broadcast_key, std::size_t eec = 0);

	/**
		 * \brief Run until a null pointer is popped off of the RawEventQueue. Throws an excpetion
		 * if expectedEventCount is set and a different number of events come off the queue.
		 * \exception std::string When the expectedEventCount is set and a different number of events come off the queue.
		 */
	void run();

private:
	/**
		 * \brief Reference to the SharedMemoryEventReceiver
		 */
	std::shared_ptr<SharedMemoryEventReceiver> incoming_events_;
	/**
		 * \brief For testing purposes, the expected number of events
		 */
	std::size_t expectedEventCount_;
};
}  // namespace artdaq

#endif /* artdaq_core_Core_SimpleMemoryReader_hh */
