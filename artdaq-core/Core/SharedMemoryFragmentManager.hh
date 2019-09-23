#ifndef ARTDAQ_CORE_CORE_SHARED_MEMORY_FRAGMENT_MANAGER_HH
#define ARTDAQ_CORE_CORE_SHARED_MEMORY_FRAGMENT_MANAGER_HH 1

#include "artdaq-core/Core/SharedMemoryManager.hh"
#include "artdaq-core/Data/RawEvent.hh"

namespace artdaq {
/**
	 * \brief The SharedMemoryFragmentManager is a SharedMemoryManager that deals with Fragment transfers using a SharedMemoryManager.
	 */
class SharedMemoryFragmentManager : public SharedMemoryManager
{
public:
	/**
		* \brief SharedMemoryFragmentManager Constructor
		* \param shm_key The key to use when attaching/creating the shared memory segment
		* \param buffer_count The number of buffers in the shared memory
		* \param max_buffer_size The size of each buffer
		* \param buffer_timeout_us The maximum amount of time a buffer may be locked
		* before being returned to its previous state. This timer is reset upon any operation by the owning SharedMemoryManager.
		*/
	SharedMemoryFragmentManager(uint32_t shm_key, size_t buffer_count = 0, size_t max_buffer_size = 0, size_t buffer_timeout_us = 100 * 1000000);

	/**
		 * \brief SharedMemoryFragmentManager destructor
		 */
	virtual ~SharedMemoryFragmentManager() = default;

	/**
		 * \brief Write a Fragment to the Shared Memory
		 * \param fragment Fragment to write
		 * \param overwrite Whether to set the overwrite flag
		 * \param timeout_us Time to wait for shared memory to be free (0: No timeout) (Timeout does not apply if overwrite == false)
		 * \return 0 on success
		 */
	int WriteFragment(Fragment&& fragment, bool overwrite, size_t timeout_us);

	/**
		 * \brief Read a Fragment from the Shared Memory
		 * \param fragment Output Fragment object
		 * \return 0 on success
		 */
	int ReadFragment(Fragment& fragment);

	/**
		 * \brief Read a Fragment Header from the Shared Memory
		 * \param header Output Fragment Header
		 * \return 0 on success
		 */
	int ReadFragmentHeader(detail::RawFragmentHeader& header);

	/**
		* \brief Read Fragment Data from the Shared Memory
		* \param destination Destination for data
		* \param words RawDataType Word count to read
		* \return 0 on success
		*/
	int ReadFragmentData(RawDataType* destination, size_t words);

	/**
		 * \brief Check if a buffer is ready for writing, and if so, reserves it for use
		 * \param overwrite Whether to overwrite Full buffers (non-reliable mode)
		 * \return True if SharedMemoryFragmentManager is ready for Fragment data
		 */
	bool ReadyForWrite(bool overwrite) override;

private:
	int active_buffer_;
};
}  // namespace artdaq

#endif  //ARTDAQ_CORE_CORE_SHARED_MEMORY_FRAGMENT_MANAGER_HH
