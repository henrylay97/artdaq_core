#ifndef artdaq_core_Core_SharedMemoryManager_hh
#define artdaq_core_Core_SharedMemoryManager_hh 1

#include <atomic>
#include <deque>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include "artdaq-core/Utilities/TimeUtils.hh"

namespace artdaq {
/**
	 * \brief The SharedMemoryManager creates a Shared Memory area which is divided into a number of fixed-size buffers.
	 * It provides for multiple readers and multiple writers through a dual semaphore system.
	 */
class SharedMemoryManager
{
public:
	/**
		 * \brief The BufferSemaphoreFlags enumeration represents the different possible "states" of a given shared memory buffer
		 */
	enum class BufferSemaphoreFlags
	{
		Empty,    ///< The buffer is empty, and waiting for a writer
		Writing,  ///< The buffer is currently being written to
		Full,     ///< The buffer is full, and waiting for a reader
		Reading   ///< The buffer is currently being read from
	};

	/**
		 * \brief Convert a BufferSemaphoreFlags variable to its string represenatation
		 * \param flag BufferSemaphoreFlags variable to convert
		 * \return String representation of flag
		 */
	static inline std::string FlagToString(BufferSemaphoreFlags flag)
	{
		switch (flag)
		{
			case BufferSemaphoreFlags::Empty:
				return "Empty";
			case BufferSemaphoreFlags::Writing:
				return "Writing";
			case BufferSemaphoreFlags::Full:
				return "Full";
			case BufferSemaphoreFlags::Reading:
				return "Reading";
		}
		return "Unknown";
	}

	/**
		 * \brief SharedMemoryManager Constructor
		 * \param shm_key The key to use when attaching/creating the shared memory segment
		 * \param buffer_count The number of buffers in the shared memory
		 * \param buffer_size The size of each buffer
		 * \param buffer_timeout_us The maximum amount of time a buffer can be left untouched by its owner (if 0, buffers do not expire)
		 * before being returned to its previous state.
		 * \param destructive_read_mode Whether a read operation empties the buffer (default: true, false for broadcast mode)
		 */
	SharedMemoryManager(uint32_t shm_key, size_t buffer_count = 0, size_t buffer_size = 0, uint64_t buffer_timeout_us = 100 * 1000000, bool destructive_read_mode = true);

	/**
		 * \brief SharedMemoryManager Destructor
		 */
	virtual ~SharedMemoryManager() noexcept;

	/**
		 * \brief Reconnect to the shared memory segment
		 */
	bool Attach(size_t timeout_usec = 0);

	/**
		 * \brief Finds a buffer that is ready to be read, and reserves it for the calling manager.
		 * \return The id number of the buffer. -1 indicates no buffers available for read.
		 */
	int GetBufferForReading();

	/**
		 * \brief Finds a buffer that is ready to be written to, and reserves it for the calling manager.
		 * \param overwrite Whether to consider buffers that are in the Full and Reading state as ready for write (non-reliable mode)
		 * \return The id number of the buffer. -1 indicates no buffers available for write.
		 */
	int GetBufferForWriting(bool overwrite);

	/**
		 * \brief Whether any buffer is ready for read
		 * \return True if there is a buffer available
		 */
	bool ReadyForRead();

	/**
		 * \brief Whether any buffer is available for write
		 * \param overwrite Whether to allow overwriting full buffers
		 * \return True if there is a buffer available
		 */
	virtual bool ReadyForWrite(bool overwrite);

	/**
		 * \brief Count the number of buffers that are ready for reading
		 * \return The number of buffers ready for reading
		 */
	size_t ReadReadyCount();

	/**
		 * \brief Count the number of buffers that are ready for writing
		 * \param overwrite Whether to consider buffers that are in the Full and Reading state as ready for write (non-reliable mode)
		 * \return The number of buffers ready for writing
		 */
	size_t WriteReadyCount(bool overwrite);

	/**
		 * \brief Get the list of all buffers currently owned by this manager instance.
         * \param locked Default = true, Whether to lock search_mutex_ before checking buffer ownership (skipped in Detach)
		 * \return A std::deque<int> of buffer IDs currently owned by this manager instance.
		 */
	std::deque<int> GetBuffersOwnedByManager(bool locked = true);

	/**
		 * \brief Get the current size of the buffer's data
		 * \param buffer Buffer ID of buffer
		 * \return Current size of data in the buffer
		 */
	size_t BufferDataSize(int buffer);

	/**
		 * \brief Get the size of of a single buffer
		 */
	size_t BufferSize() { return (shm_ptr_ != nullptr ? shm_ptr_->buffer_size : 0); }

	/**
		 * \brief Set the read position of the given buffer to the beginning of the buffer
		 * \param buffer Buffer ID of buffer
		 */
	void ResetReadPos(int buffer);

	/**
		 * \brief Set the write position of the given buffer to the beginning of the buffer
		 * \param buffer Buffer ID of buffer
		 */
	void ResetWritePos(int buffer);
	/**
		 * \brief Increment the read position for a given buffer
		 * \param buffer Buffer ID of buffer
		 * \param read Number of bytes by which to increment read position
		 */
	void IncrementReadPos(int buffer, size_t read);

	/**
		* \brief Increment the write position for a given buffer
		* \param buffer Buffer ID of buffer
		* \param written Number of bytes by which to increment write position
		* \return Whether the write is allowed
		*/
	bool IncrementWritePos(int buffer, size_t written);

	/**
		 * \brief Determine if more data is available to be read, based on the read position and data size
		 * \param buffer Buffer ID of buffer
		 * \return Whether more data is available in the given buffer.
		 */
	bool MoreDataInBuffer(int buffer);

	/**
		 * \brief Check both semaphore conditions (Mode flag and manager ID) for a given buffer
		 * \param buffer Buffer ID of buffer
		 * \param flags Expected Mode flag
		 * \return Whether the buffer passed the check
		 */
	bool CheckBuffer(int buffer, BufferSemaphoreFlags flags);

	/**
		 * \brief Release a buffer from a writer, marking it Full and ready for a reader
		 * \param buffer Buffer ID of buffer
		 * \param destination If desired, a destination manager ID may be specified for a buffer
		 */
	void MarkBufferFull(int buffer, int destination = -1);

	/**
		 * \brief Release a buffer from a reader, marking it Empty and ready to accept more data
		 * \param buffer Buffer ID of buffer
		 * \param force Force buffer to empty state (only if manager_id_ == 0)
		 */
	void MarkBufferEmpty(int buffer, bool force = false);

	/**
		 * \brief Resets the buffer from Reading to Full. This operation will only have an
		 * effect if performed by the owning manager or if the buffer has timed out.
		 * \param buffer Buffer ID of buffer
		 * \return Whether the buffer has exceeded the maximum age
		 */
	bool ResetBuffer(int buffer);

	/**
		 * \brief Assign a new ID to the current SharedMemoryManager, if one has not yet been assigned
		 */
	void GetNewId()
	{
		if (manager_id_ < 0 && IsValid()) manager_id_ = shm_ptr_->next_id.fetch_add(1);
	}

	/**
		 * \brief Get the number of attached SharedMemoryManagers
		 * \return The number of attached SharedMemoryManagers
		 */
	uint16_t GetAttachedCount() const;

	/**
		 * \brief Reset the attached manager count to 0
		 */
	void ResetAttachedCount() const
	{
		if (manager_id_ == 0 && IsValid()) shm_ptr_->next_id = 1;
	}

	/**
		 * \brief Get the ID number of the current SharedMemoryManager
		 * \return The ID number of the current SharedMemoryManager
		 */
	int GetMyId() const { return manager_id_; }

	/**
		 * \brief Get the rank of the owner of the Shared Memory (artdaq assigns rank to each artdaq process for data flow)
		 * \return The rank of the owner of the Shared Memory
		 */
	int GetRank() const { return IsValid() ? shm_ptr_->rank : -1; }

	/**
		 * \brief Set the rank stored in the Shared Memory, if the current instance is the owner of the shared memory
		 * \param rank Rank to set
		 */
	void SetRank(int rank)
	{
		if (manager_id_ == 0 && IsValid()) shm_ptr_->rank = rank;
	}

	/**
		 * \brief Is the shared memory pointer valid?
		 * \return Whether the shared memory pointer is valid
		 */
	bool IsValid() const { return shm_ptr_ ? true : false; }

	/**
		 * \brief Determine whether the Shared Memory is marked for destruction (End of Data)
		 */
	bool IsEndOfData() const;

	/**
		 * \brief Get the number of buffers in the shared memory segment
		 * \return The number of buffers in the shared memory segment
		 */
	size_t size() const { return IsValid() ? shm_ptr_->buffer_count : 0; }

	/**
		 * \brief Write size bytes of data from the given pointer to a buffer
		 * \param buffer Buffer ID of buffer
		 * \param data Source pointer for write
		 * \param size Size of write, in bytes
		 * \return Amount of data written, in bytes
		 */
	size_t Write(int buffer, void* data, size_t size);

	/**
		 * \brief Read size bytes of data from buffer into the given pointer
		 * \param buffer Buffer ID of buffer
		 * \param data Destination pointer for read
		 * \param size Size of read, in bytes
		 * \return Whether the read was successful
		 */
	bool Read(int buffer, void* data, size_t size);

	/**
		 *\brief Write information about the SharedMemory to a string
		 *\return String describing current state of SharedMemory and buffers
		 */
	virtual std::string toString();

	/**
		 * \brief Get the key of the shared memory attached to this SharedMemoryManager
		 * \return The shared memory key
		 */
	uint32_t GetKey() const { return shm_key_; }

	/**
		 * \brief Get a pointer to the current read position of the buffer
		 * \param buffer Buffer ID of buffer
		 * \return void* pointer to the buffer's current read position
		 */
	void* GetReadPos(int buffer);

	/**
		 * \brief Get a pointer to the current write position of the buffer
		 * \param buffer Buffer ID of buffer
		 * \return void* pointer to buffer's current write position
		 */
	void* GetWritePos(int buffer);

	/**
		 * \brief Get a pointer to the start position of the buffer
		 * \param buffer Buffer ID of buffer
		 * \return void* pointer to buffer start position
		 */
	void* GetBufferStart(int buffer);

	/**
		 * \brief Detach from the Shared Memory segment, optionally throwing a cet::exception with the specified properties
		 * \param throwException Whether to throw an exception after detaching
		 * \param category Category for the cet::exception
		 * \param message Message for the cet::exception
		 * \param force Whether to mark shared memory for destruction even if not owner (i.e. from signal handler)
		 */
	void Detach(bool throwException = false, std::string category = "", std::string message = "", bool force = false);

	/**
		 * \brief Gets the configured timeout for buffers to be declared "stale"
		 * \return The buffer timeout, in microseconds
		 */
	uint64_t GetBufferTimeout() const { return IsValid() ? shm_ptr_->buffer_timeout_us : 0; }

	/**
		 * \brief Gets the number of buffers which have been processed through the Shared Memory
		 * \return The number of buffers processed by the Shared Memory
		 */
	size_t GetBufferCount() const { return IsValid() ? shm_ptr_->next_sequence_id : 0; }

	/**
		 * \brief Gets the highest buffer number either written or read by this SharedMemoryManager
		 * \return The highest buffer id written or read by this SharedMemoryManager
		 */
	size_t GetLastSeenBufferID() const { return last_seen_id_; }

	/**
		 * \brief Gets the lowest sequence ID that has been read by any reader, as reported by the readers.
		 */
	size_t GetLowestSeqIDRead() const { return IsValid() ? shm_ptr_->lowest_seq_id_read : 0; }

	/**
		 * \brief Sets the threshold after which a buffer should be considered "non-empty" (in case of default headers)
		 * \param size Size (in bytes) after which a buffer will be considered non-empty
		 */
	void SetMinWriteSize(size_t size) { min_write_size_ = size; }

	/**
		 * \brief Get a report on the status of each buffer
		 * \return A list of manager_id, semaphore pairs
		 */
	std::list<std::pair<int, BufferSemaphoreFlags>> GetBufferReport();

	/**
		 * \brief Touch the given buffer (update its last_touch_time)
		 */
	void TouchBuffer(int buffer) { return touchBuffer_(getBufferInfo_(buffer)); }

private:
	struct ShmBuffer
	{
		size_t writePos;
		size_t readPos;
		std::atomic<BufferSemaphoreFlags> sem;
		std::atomic<int16_t> sem_id;
		std::atomic<size_t> sequence_id;
		std::atomic<uint64_t> last_touch_time;
	};

	struct ShmStruct
	{
		std::atomic<unsigned int> reader_pos;
		std::atomic<unsigned int> writer_pos;
		int buffer_count;
		size_t buffer_size;
		size_t buffer_timeout_us;
		size_t next_sequence_id;
		size_t lowest_seq_id_read;
		bool destructive_read_mode;

		std::atomic<int> next_id;
		int rank;
		unsigned ready_magic;
	};

	uint8_t* dataStart_() const;
	uint8_t* bufferStart_(int buffer);
	ShmBuffer* getBufferInfo_(int buffer);
	bool checkBuffer_(ShmBuffer* buffer, BufferSemaphoreFlags flags, bool exceptions = true);
	void touchBuffer_(ShmBuffer* buffer);

	ShmStruct requested_shm_parameters_;

	int shm_segment_id_;
	ShmStruct* shm_ptr_;
	uint32_t shm_key_;
	int manager_id_;
	mutable std::vector<std::mutex> buffer_mutexes_;
	mutable std::mutex search_mutex_;

	std::atomic<size_t> last_seen_id_;
	size_t min_write_size_;
};

}  // namespace artdaq

#endif  // artdaq_core_Core_SharedMemoryManager_hh
