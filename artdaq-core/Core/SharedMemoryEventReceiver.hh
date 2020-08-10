#ifndef artdaq_core_Core_SharedMemoryEventReceiver_hh
#define artdaq_core_Core_SharedMemoryEventReceiver_hh 1

#include <set>

#include "artdaq-core/Core/SharedMemoryManager.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/RawEvent.hh"

namespace artdaq {
/**
	 * \brief SharedMemoryEventReceiver can receive events (as written by SharedMemoryEventManager) from Shared Memory
	 */
class SharedMemoryEventReceiver
{
public:
	/**
		 * \brief Connect to a Shared Memory segment using the given parameters
		 * \param shm_key Key of the Shared Memory segment
		 * \param broadcast_shm_key Key of the broadcast Shared Memory segment
		 */
	SharedMemoryEventReceiver(uint32_t shm_key, uint32_t broadcast_shm_key);
	/**
		 * \brief SharedMemoryEventReceiver Destructor
		 */
	virtual ~SharedMemoryEventReceiver() = default;

	/**
		 * \brief Determine whether an event is available for reading
		 * \param broadcast (Default false) Whether to wait for a broadcast buffer only
		 * \param timeout_us (Default 1000000) Time to wait for buffer to become available.
		 * \return Whether an event is available for reading
		 */
	bool ReadyForRead(bool broadcast = false, size_t timeout_us = 1000000);

	/**
		 * \brief Get the Event header
		 * \param err Flag used to indicate if an error has occurred
		 * \return Pointer to RawEventHeader from buffer
		 */
	detail::RawEventHeader* ReadHeader(bool& err);

	/**
		 * \brief Get a set of Fragment Types present in the event
		 * \param err Flag used to indicate if an error has occurred
		 * \return std::set of Fragment::type_t of all Fragment types in the event
		 */
	std::set<Fragment::type_t> GetFragmentTypes(bool& err);

	/**
		 * \brief Get a pointer to the Fragments of a given type in the event
		 * \param err Flag used to indicate if an error has occurred
		 * \param type Type of Fragments to get. (Use InvalidFragmentType to get all Fragments)
		 * \return std::unique_ptr to a Fragments object containing returned Fragment objects
		 */
	std::unique_ptr<Fragments> GetFragmentsByType(bool& err, Fragment::type_t type);

	/**
		* \brief Write out information about the Shared Memory to a string
		* \return String containing information about the current Shared Memory buffers
		*/
	std::string toString();

	/**
		 * \brief Release the buffer currently being read to the Empty state
		 */
	void ReleaseBuffer();

	/**
		 * \brief Returns the Rank of the writing process
		 * \return The rank of the writer process
		 */
	int GetRank() { return data_.GetRank(); }

	/**
		 * \brief Returns the ID of the reading process
		 * \return The ID of the reading process
		 */
	int GetMyId() { return data_.GetMyId(); }

	/**
	 * \brief Determine if the end of data has been reached (from data shared memory segment)
	 * \return Whether the EndOfData flag has been raised by the data shared memory segment
	 */
	bool IsEndOfData() { return data_.IsEndOfData(); }

	/**
		 * \brief Get the count of available buffers, both broadcasts and data
		 * \return The sum of the available data buffer count and the available broadcast buffer count
		 */
	int ReadReadyCount() { return data_.ReadReadyCount() + broadcasts_.ReadReadyCount(); }

	/**
		 * \brief Get the size of the data buffer
		 * \return The size of the data buffer
		 */
	size_t size() { return data_.size(); }

private:
	SharedMemoryEventReceiver(SharedMemoryEventReceiver const&) = delete;
	SharedMemoryEventReceiver(SharedMemoryEventReceiver&&) = delete;
	SharedMemoryEventReceiver& operator=(SharedMemoryEventReceiver const&) = delete;
	SharedMemoryEventReceiver& operator=(SharedMemoryEventReceiver&&) = delete;

	std::string printBuffers_(SharedMemoryManager* data_source);

	int current_read_buffer_;
	bool initialized_;
	detail::RawEventHeader* current_header_;
	SharedMemoryManager* current_data_source_;
	SharedMemoryManager data_;
	SharedMemoryManager broadcasts_;
};
}  // namespace artdaq

#endif /* artdaq_core_Core_SharedMemoryEventReceiver_hh */
