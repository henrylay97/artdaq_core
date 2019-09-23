#define TRACE_NAME "SharedMemoryManager"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <list>
#include <unordered_map>
#ifndef SHM_DEST  // Lynn reports that this is missing on Mac OS X?!?
#define SHM_DEST 01000
#endif
#include <signal.h>
#include "artdaq-core/Core/SharedMemoryManager.hh"
#include "artdaq-core/Utilities/TraceLock.hh"
#include "cetlib_except/exception.h"
#include "tracemf.h"

#define TLVL_DETACH 11
#define TLVL_BUFFER 40
#define TLVL_BUFLCK 41

static std::list<artdaq::SharedMemoryManager const*> instances = std::list<artdaq::SharedMemoryManager const*>();

static std::unordered_map<int, struct sigaction> old_actions = std::unordered_map<int, struct sigaction>();
static bool sighandler_init = false;
static void signal_handler(int signum)
{
	// Messagefacility may already be gone at this point, TRACE ONLY!
	TRACE_STREAMER(TLVL_ERROR, &("SharedMemoryManager")[0], 0, 0, 0) << "A signal of type " << signum << " was caught by SharedMemoryManager. Detaching all Shared Memory segments, then proceeding with default handlers!";
	for (auto ii : instances)
	{
		if (ii)
		{
			const_cast<artdaq::SharedMemoryManager*>(ii)->Detach(false, "", "", false /* don't force destruct segment, allows reconnection (applicable for
						   restart and/or multiple art processes (i.e. dispatcher)) */
			);
		}
		ii = nullptr;
	}

	sigset_t set;
	pthread_sigmask(SIG_UNBLOCK, NULL, &set);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);

	TRACE_STREAMER(TLVL_ERROR, &("SharedMemoryManager")[0], 0, 0, 0) << "Calling default signal handler";
	if (signum != SIGUSR2)
	{
		sigaction(signum, &old_actions[signum], NULL);
		kill(getpid(), signum);  // Only send signal to self
	}
	else
	{
		// Send Interrupt signal if parsing SIGUSR2 (i.e. user-defined exception that should tear down ARTDAQ)
		sigaction(SIGINT, &old_actions[SIGINT], NULL);
		kill(getpid(), SIGINT);  // Only send signal to self
	}
}

artdaq::SharedMemoryManager::SharedMemoryManager(uint32_t shm_key, size_t buffer_count, size_t buffer_size, uint64_t buffer_timeout_us, bool destructive_read_mode)
    : shm_segment_id_(-1)
    , shm_ptr_(NULL)
    , shm_key_(shm_key)
    , manager_id_(-1)
    , buffer_mutexes_()
    , last_seen_id_(0)
{
	requested_shm_parameters_.buffer_count = buffer_count;
	requested_shm_parameters_.buffer_size = buffer_size;
	requested_shm_parameters_.buffer_timeout_us = buffer_timeout_us;
	requested_shm_parameters_.destructive_read_mode = destructive_read_mode;

	instances.push_back(this);
	Attach();

	static std::mutex sighandler_mutex;
	std::unique_lock<std::mutex> lk(sighandler_mutex);

	if (!sighandler_init)  //&& manager_id_ == 0) // ELF 3/22/18: Taking out manager_id_==0 requirement as I think kill(getpid()) is enough protection
	{
		sighandler_init = true;
		std::vector<int> signals = {SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR2, SIGHUP};  // SIGQUIT is used by art in normal operation
		for (auto signal : signals)
		{
			struct sigaction old_action;
			sigaction(signal, NULL, &old_action);

			//If the old handler wasn't SIG_IGN (it's a handler that just
			// "ignore" the signal)
			if (old_action.sa_handler != SIG_IGN)
			{
				struct sigaction action;
				action.sa_handler = signal_handler;
				sigemptyset(&action.sa_mask);
				for (auto sigblk : signals)
				{
					sigaddset(&action.sa_mask, sigblk);
				}
				action.sa_flags = 0;

				//Replace the signal handler of SIGINT with the one described by new_action
				sigaction(signal, &action, NULL);
				old_actions[signal] = old_action;
			}
		}
	}
}

artdaq::SharedMemoryManager::~SharedMemoryManager() noexcept
{
	{
		static std::mutex destructor_mutex;
		std::lock_guard<std::mutex> lk(destructor_mutex);
		for (auto it = instances.begin(); it != instances.end(); ++it)
		{
			if (*it == this)
			{
				it = instances.erase(it);
				break;
			}
		}
	}
	TLOG(TLVL_DEBUG) << "~SharedMemoryManager called";
	Detach();
	TLOG(TLVL_DEBUG) << "~SharedMemoryManager done";
}

bool artdaq::SharedMemoryManager::Attach(size_t timeout_usec)
{
	if (IsValid())
	{
		if (manager_id_ == 0) return true;
		Detach();
	}

	size_t timeout_us = timeout_usec > 0 ? timeout_usec : 1000000;
	auto start_time = std::chrono::steady_clock::now();
	last_seen_id_ = 0;
	size_t shmSize = requested_shm_parameters_.buffer_count * (requested_shm_parameters_.buffer_size + sizeof(ShmBuffer)) + sizeof(ShmStruct);

	// 19-Feb-2019, KAB: separating out the determination of whether a given process owns the shared
	// memory (indicated by manager_id_ == 0) and whether or not the shared memory already exists.
	if (requested_shm_parameters_.buffer_count > 0 && manager_id_ <= 0)
	{
		manager_id_ = 0;
	}

	bool mark_shm_for_removal = false;
	shm_segment_id_ = shmget(shm_key_, shmSize, 0666);
	if (shm_segment_id_ == -1)
	{
		if (manager_id_ == 0)
		{
			TLOG(TLVL_DEBUG) << "Creating shared memory segment with key 0x" << std::hex << shm_key_ << " and size " << std::dec << shmSize;
			shm_segment_id_ = shmget(shm_key_, shmSize, IPC_CREAT | 0666);

			if (shm_segment_id_ == -1)
			{
				TLOG(TLVL_ERROR) << "Error creating shared memory segment with key 0x" << std::hex << shm_key_ << ", errno=" << std::dec << errno << " (" << strerror(errno) << ")";
			}
		}
		else
		{
			TLOG(TLVL_WARNING) << "This process is re-using an existing shared memory segment with key 0x" << std::hex << shm_key_ << " and ID " << std::dec << shm_segment_id_ << " instead of creating a new one. This may or may not work. If stale data is transfered, please try again. This shared memory segment will be marked for removal once all processes disconnect, so hopefully, this problem will not persist.";
			mark_shm_for_removal = true;
		}
	}
	else
	{
		while (shm_segment_id_ == -1 && TimeUtils::GetElapsedTimeMilliseconds(start_time) < 1000)
		{
			shm_segment_id_ = shmget(shm_key_, shmSize, 0666);
		}
	}
	TLOG(TLVL_DEBUG) << "shm_key == 0x" << std::hex << shm_key_ << ", shm_segment_id == " << std::dec << shm_segment_id_;

	if (shm_segment_id_ > -1)
	{
		TLOG(TLVL_DEBUG)
		    << "Attached to shared memory segment with ID = " << shm_segment_id_
		    << " and size " << shmSize
		    << " bytes";
		shm_ptr_ = (ShmStruct*)shmat(shm_segment_id_, 0, 0);
		TLOG(TLVL_DEBUG)
		    << "Attached to shared memory segment at address "
		    << std::hex << (void*)shm_ptr_ << std::dec;
		if (shm_ptr_ && shm_ptr_ != (void*)-1)
		{
			if (manager_id_ == 0)
			{
				if (shm_ptr_->ready_magic == 0xCAFE1111)
				{
					TLOG(TLVL_WARNING) << "Owner encountered already-initialized Shared Memory! "
					                   << "Once the system is shut down, you can use one of the following commands "
					                   << "to clean up this shared memory: 'ipcrm -M 0x" << std::hex << shm_key_
					                   << "' or 'ipcrm -m " << std::dec << shm_segment_id_ << "'.";
					//exit(-2);
				}
				TLOG(TLVL_DEBUG) << "Owner initializing Shared Memory";
				shm_ptr_->next_id = 1;
				shm_ptr_->next_sequence_id = 0;
				shm_ptr_->reader_pos = 0;
				shm_ptr_->writer_pos = 0;
				shm_ptr_->buffer_size = requested_shm_parameters_.buffer_size;
				shm_ptr_->buffer_count = requested_shm_parameters_.buffer_count;
				shm_ptr_->buffer_timeout_us = requested_shm_parameters_.buffer_timeout_us;
				shm_ptr_->destructive_read_mode = requested_shm_parameters_.destructive_read_mode;

				for (int ii = 0; ii < static_cast<int>(requested_shm_parameters_.buffer_count); ++ii)
				{
					if (!getBufferInfo_(ii)) return false;
					getBufferInfo_(ii)->writePos = 0;
					getBufferInfo_(ii)->readPos = 0;
					getBufferInfo_(ii)->sem = BufferSemaphoreFlags::Empty;
					getBufferInfo_(ii)->sem_id = -1;
					getBufferInfo_(ii)->last_touch_time = TimeUtils::gettimeofday_us();
				}

				shm_ptr_->ready_magic = 0xCAFE1111;
			}
			else
			{
				TLOG(TLVL_DEBUG) << "Waiting for owner to initalize Shared Memory";
				while (shm_ptr_->ready_magic != 0xCAFE1111) { usleep(1000); }
				TLOG(TLVL_DEBUG) << "Getting ID from Shared Memory";
				GetNewId();
				shm_ptr_->lowest_seq_id_read = 0;
				TLOG(TLVL_DEBUG) << "Getting Shared Memory Size parameters";
			}
			//last_seen_id_ = shm_ptr_->next_sequence_id;
			buffer_mutexes_ = std::vector<std::mutex>(shm_ptr_->buffer_count);
			TLOG(TLVL_DEBUG) << "Initialization Complete: "
			                 << "key: 0x" << std::hex << shm_key_
			                 << ", manager ID: " << std::dec << manager_id_
			                 << ", Buffer size: " << shm_ptr_->buffer_size
			                 << ", Buffer count: " << shm_ptr_->buffer_count;

			// If this process is the owner of the shared memory segment, but it didn't create
			// it in this session, mark it for removal after all processes have disconnected.
			// We'll continue to use the segment in this session, in the hope that it works,
			// but if it doesn't, the future removal should help.
			if (mark_shm_for_removal)
			{
				shmctl(shm_segment_id_, IPC_RMID, NULL);
			}

			return true;
		}
		else
		{
			TLOG(TLVL_ERROR) << "Failed to attach to shared memory segment "
			                 << shm_segment_id_;
			return false;
		}
	}
	else
	{
		TLOG(TLVL_ERROR) << "Failed to connect to shared memory segment with key 0x" << std::hex << shm_key_
		                 << ", errno=" << std::dec << errno << " (" << strerror(errno) << ")"
		                 << ".  Please check "
		                 << "if a stale shared memory segment needs to "
		                 << "be cleaned up. (ipcs, ipcrm -m <segId>)";
		return false;
	}

	// Should not get here...
	return false;
}

int artdaq::SharedMemoryManager::GetBufferForReading()
{
	TLOG(13) << "GetBufferForReading BEGIN";

	std::unique_lock<std::mutex> lk(search_mutex_);
	//TraceLock lk(search_mutex_, 11, "GetBufferForReadingSearch");
	auto rp = shm_ptr_->reader_pos.load();

	TLOG(13) << "GetBufferForReading lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";

	for (int retry = 0; retry < 5; retry++)
	{
		BufferSemaphoreFlags sem;
		short sem_id;
		int buffer_num = -1;
		ShmBuffer* buffer_ptr = nullptr;
		uint64_t seqID = -1;

		for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
		{
			auto buffer = (ii + rp) % shm_ptr_->buffer_count;

			TLOG(14) << "GetBufferForReading Checking if buffer " << buffer << " is stale. Shm destructive_read_mode=" << shm_ptr_->destructive_read_mode;
			ResetBuffer(buffer);

			auto buf = getBufferInfo_(buffer);
			if (!buf) continue;

			sem = buf->sem.load();
			sem_id = buf->sem_id.load();

			TLOG(14) << "GetBufferForReading: Buffer " << buffer << ": sem=" << FlagToString(sem)
			         << " (expected " << FlagToString(BufferSemaphoreFlags::Full) << "), sem_id=" << sem_id << ", seq_id=" << buf->sequence_id << " )";
			if (sem == BufferSemaphoreFlags::Full && (sem_id == -1 || sem_id == manager_id_) && (shm_ptr_->destructive_read_mode || buf->sequence_id > last_seen_id_))
			{
				if (buf->sequence_id < seqID)
				{
					buffer_ptr = buf;
					seqID = buf->sequence_id;
					buffer_num = buffer;
					touchBuffer_(buf);
					if (shm_ptr_->destructive_read_mode || seqID == last_seen_id_ + 1) break;
				}
			}
		}

		if (buffer_ptr)
		{
			sem = buffer_ptr->sem.load();
			sem_id = buffer_ptr->sem_id.load();
		}

		if (!buffer_ptr || (sem_id != -1 && sem_id != manager_id_) || sem != BufferSemaphoreFlags::Full)
		{
			continue;
		}

		if (buffer_num >= 0)
		{
			TLOG(13) << "GetBufferForReading Found buffer " << buffer_num;
			touchBuffer_(buffer_ptr);
			if (!buffer_ptr->sem_id.compare_exchange_strong(sem_id, manager_id_)) continue;
			if (!buffer_ptr->sem.compare_exchange_strong(sem, BufferSemaphoreFlags::Reading)) continue;
			if (!checkBuffer_(buffer_ptr, BufferSemaphoreFlags::Reading, false))
			{
				TLOG(13) << "GetBufferForReading: Failed to acquire buffer " << buffer_num << " (someone else changed manager ID while I was changing sem)";
				continue;
			}
			buffer_ptr->readPos = 0;
			touchBuffer_(buffer_ptr);
			if (!checkBuffer_(buffer_ptr, BufferSemaphoreFlags::Reading, false))
			{
				TLOG(13) << "GetBufferForReading: Failed to acquire buffer " << buffer_num << " (someone else changed manager ID while I was touching buffer SHOULD NOT HAPPEN!)";
				continue;
			}
			if (shm_ptr_->destructive_read_mode && shm_ptr_->lowest_seq_id_read == last_seen_id_)
			{
				shm_ptr_->lowest_seq_id_read = seqID;
			}
			last_seen_id_ = seqID;
			if (shm_ptr_->destructive_read_mode) shm_ptr_->reader_pos = (buffer_num + 1) % shm_ptr_->buffer_count;

			TLOG(13) << "GetBufferForReading returning " << buffer_num;
			return buffer_num;
		}
		retry = 5;
	}

	TLOG(13) << "GetBufferForReading returning -1 because no buffers are ready";
	return -1;
}

int artdaq::SharedMemoryManager::GetBufferForWriting(bool overwrite)
{
	TLOG(14) << "GetBufferForWriting BEGIN, overwrite=" << (overwrite ? "true" : "false");

	std::unique_lock<std::mutex> lk(search_mutex_);
	//TraceLock lk(search_mutex_, 12, "GetBufferForWritingSearch");
	auto wp = shm_ptr_->writer_pos.load();

	TLOG(13) << "GetBufferForWriting lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";

	// First, only look for "Empty" buffers
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (ii + wp) % shm_ptr_->buffer_count;

		ResetBuffer(buffer);

		auto buf = getBufferInfo_(buffer);
		if (!buf) continue;

		auto sem = buf->sem.load();
		auto sem_id = buf->sem_id.load();

		if (sem == BufferSemaphoreFlags::Empty && sem_id == -1)
		{
			touchBuffer_(buf);
			if (!buf->sem_id.compare_exchange_strong(sem_id, manager_id_)) continue;
			if (!buf->sem.compare_exchange_strong(sem, BufferSemaphoreFlags::Writing)) continue;
			if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
			shm_ptr_->writer_pos = (buffer + 1) % shm_ptr_->buffer_count;
			buf->sequence_id = ++shm_ptr_->next_sequence_id;
			buf->writePos = 0;
			if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
			touchBuffer_(buf);
			TLOG(14) << "GetBufferForWriting returning " << buffer;
			return buffer;
		}
	}

	if (overwrite)
	{
		// Then, look for "Full" buffers
		for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
		{
			auto buffer = (ii + wp) % shm_ptr_->buffer_count;

			ResetBuffer(buffer);

			auto buf = getBufferInfo_(buffer);
			if (!buf) continue;

			auto sem = buf->sem.load();
			auto sem_id = buf->sem_id.load();

			if (sem == BufferSemaphoreFlags::Full)
			{
				touchBuffer_(buf);
				if (!buf->sem_id.compare_exchange_strong(sem_id, manager_id_)) continue;
				if (!buf->sem.compare_exchange_strong(sem, BufferSemaphoreFlags::Writing)) continue;
				if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
				shm_ptr_->writer_pos = (buffer + 1) % shm_ptr_->buffer_count;
				buf->sequence_id = ++shm_ptr_->next_sequence_id;
				buf->writePos = 0;
				if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
				touchBuffer_(buf);
				TLOG(14) << "GetBufferForWriting returning " << buffer;
				return buffer;
			}
		}

		// Finally, if we still haven't found a buffer, we have to clobber a reader...
		for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
		{
			auto buffer = (ii + wp) % shm_ptr_->buffer_count;

			ResetBuffer(buffer);

			auto buf = getBufferInfo_(buffer);
			if (!buf) continue;

			auto sem = buf->sem.load();
			auto sem_id = buf->sem_id.load();

			if (sem == BufferSemaphoreFlags::Reading)
			{
				touchBuffer_(buf);
				if (!buf->sem_id.compare_exchange_strong(sem_id, manager_id_)) continue;
				if (!buf->sem.compare_exchange_strong(sem, BufferSemaphoreFlags::Writing)) continue;
				if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
				shm_ptr_->writer_pos = (buffer + 1) % shm_ptr_->buffer_count;
				buf->sequence_id = ++shm_ptr_->next_sequence_id;
				buf->writePos = 0;
				if (!checkBuffer_(buf, BufferSemaphoreFlags::Writing, false)) continue;
				touchBuffer_(buf);
				TLOG(14) << "GetBufferForWriting returning " << buffer;
				return buffer;
			}
		}
	}
	TLOG(14) << "GetBufferForWriting Returning -1 because no buffers are ready";
	return -1;
}

size_t artdaq::SharedMemoryManager::ReadReadyCount()
{
	if (!IsValid()) return 0;
	TLOG(23) << "0x" << std::hex << shm_key_ << " ReadReadyCount BEGIN" << std::dec;
	std::unique_lock<std::mutex> lk(search_mutex_);
	TLOG(23) << "ReadReadyCount lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";
	//TraceLock lk(search_mutex_, 14, "ReadReadyCountSearch");
	size_t count = 0;
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		TLOG(24) << "0x" << std::hex << shm_key_ << std::dec << " ReadReadyCount: Checking if buffer " << ii << " is stale.";
		ResetBuffer(ii);
		auto buf = getBufferInfo_(ii);
		if (!buf) continue;
		TLOG(25) << "0x" << std::hex << shm_key_ << std::dec << " ReadReadyCount: Buffer " << ii << ": sem=" << FlagToString(buf->sem) << " (expected " << FlagToString(BufferSemaphoreFlags::Full) << "), sem_id=" << buf->sem_id << " )";
		if (buf->sem == BufferSemaphoreFlags::Full && (buf->sem_id == -1 || buf->sem_id == manager_id_) && (shm_ptr_->destructive_read_mode || buf->sequence_id > last_seen_id_))
		{
			TLOG(26) << "0x" << std::hex << shm_key_ << std::dec << " ReadReadyCount: Buffer " << ii << " is either unowned or owned by this manager, and is marked full.";
			touchBuffer_(buf);
			++count;
		}
	}
	return count;
}

size_t artdaq::SharedMemoryManager::WriteReadyCount(bool overwrite)
{
	if (!IsValid()) return 0;
	TLOG(28) << "0x" << std::hex << shm_key_ << " ReadReadyCount BEGIN" << std::dec;
	std::unique_lock<std::mutex> lk(search_mutex_);
	//TraceLock lk(search_mutex_, 15, "WriteReadyCountSearch");
	TLOG(28) << "WriteReadyCount(" << overwrite << ") lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";
	size_t count = 0;
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		TLOG(29) << "0x" << std::hex << shm_key_ << std::dec << " WriteReadyCount: Checking if buffer " << ii << " is stale.";
		ResetBuffer(ii);
		auto buf = getBufferInfo_(ii);
		if (!buf) continue;
		if ((buf->sem == BufferSemaphoreFlags::Empty && buf->sem_id == -1) || (overwrite && buf->sem != BufferSemaphoreFlags::Writing))
		{
			TLOG(29) << "0x" << std::hex << shm_key_ << std::dec << " WriteReadyCount: Buffer " << ii << " is either empty or is available for overwrite.";
			++count;
		}
	}
	return count;
}

bool artdaq::SharedMemoryManager::ReadyForRead()
{
	if (!IsValid()) return false;
	TLOG(23) << "0x" << std::hex << shm_key_ << " ReadyForRead BEGIN" << std::dec;
	std::unique_lock<std::mutex> lk(search_mutex_);
	//TraceLock lk(search_mutex_, 14, "ReadyForReadSearch");

	auto rp = shm_ptr_->reader_pos.load();

	TLOG(23) << "ReadyForRead lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";

	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (rp + ii) % shm_ptr_->buffer_count;
		TLOG(24) << "0x" << std::hex << shm_key_ << std::dec << " ReadyForRead: Checking if buffer " << buffer << " is stale.";
		ResetBuffer(buffer);
		auto buf = getBufferInfo_(buffer);
		if (!buf) continue;
		TLOG(25) << "0x" << std::hex << shm_key_ << std::dec << " ReadyForRead: Buffer " << buffer << ": sem=" << FlagToString(buf->sem) << " (expected " << FlagToString(BufferSemaphoreFlags::Full) << "), sem_id=" << buf->sem_id << " )"
		         << " seq_id=" << buf->sequence_id << " >? " << last_seen_id_;
		if (buf->sem == BufferSemaphoreFlags::Full && (buf->sem_id == -1 || buf->sem_id == manager_id_) && (shm_ptr_->destructive_read_mode || buf->sequence_id > last_seen_id_))
		{
			TLOG(26) << "0x" << std::hex << shm_key_ << std::dec << " ReadyForRead: Buffer " << buffer << " is either unowned or owned by this manager, and is marked full.";
			touchBuffer_(buf);
			return true;
		}
	}
	return false;
}

bool artdaq::SharedMemoryManager::ReadyForWrite(bool overwrite)
{
	if (!IsValid()) return false;
	TLOG(28) << "0x" << std::hex << shm_key_ << " ReadyForWrite BEGIN" << std::dec;

	std::unique_lock<std::mutex> lk(search_mutex_);
	//TraceLock lk(search_mutex_, 15, "ReadyForWriteSearch");

	auto wp = shm_ptr_->writer_pos.load();

	TLOG(28) << "ReadyForWrite lock acquired, scanning " << shm_ptr_->buffer_count << " buffers";

	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (wp + ii) % shm_ptr_->buffer_count;
		TLOG(29) << "0x" << std::hex << shm_key_ << std::dec << " ReadyForWrite: Checking if buffer " << buffer << " is stale.";
		ResetBuffer(buffer);
		auto buf = getBufferInfo_(buffer);
		if (!buf) continue;
		if ((buf->sem == BufferSemaphoreFlags::Empty && buf->sem_id == -1) || (overwrite && buf->sem != BufferSemaphoreFlags::Writing))
		{
			TLOG(29) << "0x" << std::hex << shm_key_
			         << std::dec
			         << " WriteReadyCount: Buffer " << ii << " is either empty or available for overwrite.";
			return true;
		}
	}
	return false;
}

std::deque<int> artdaq::SharedMemoryManager::GetBuffersOwnedByManager(bool locked)
{
	std::deque<int> output;
	if (!IsValid()) return output;
	TLOG(TLVL_BUFFER) << "GetBuffersOwnedByManager BEGIN. Locked? " << locked;
	if (locked)
	{
		TLOG(TLVL_BUFLCK) << "GetBuffersOwnedByManager obtaining search_mutex";
		std::unique_lock<std::mutex> lk(search_mutex_);
		TLOG(TLVL_BUFLCK) << "GetBuffersOwnedByManager obtained search_mutex";
		//TraceLock lk(search_mutex_, 16, "GetOwnedSearch");
		for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
		{
			auto buf = getBufferInfo_(ii);
			if (!buf) continue;
			if (buf->sem_id == manager_id_)
			{
				output.push_back(ii);
			}
		}
	}
	else
	{
		for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
		{
			auto buf = getBufferInfo_(ii);
			if (!buf) continue;
			if (buf->sem_id == manager_id_)
			{
				output.push_back(ii);
			}
		}
	}

	TLOG(TLVL_BUFFER) << "GetBuffersOwnedByManager: own " << output.size() << " / " << shm_ptr_->buffer_count << " buffers.";
	return output;
}

size_t artdaq::SharedMemoryManager::BufferDataSize(int buffer)
{
	TLOG(TLVL_BUFFER) << "BufferDataSize(" << buffer << ") called.";

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "BufferDataSize obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "BufferDataSize obtained buffer_mutex for buffer " << buffer;
	//TraceLock lk(buffer_mutexes_[buffer], 17, "DataSizeBuffer" + std::to_string(buffer));

	auto buf = getBufferInfo_(buffer);
	if (!buf) return 0;
	touchBuffer_(buf);

	TLOG(TLVL_BUFFER) << "BufferDataSize: buffer " << buffer << ", size=" << buf->writePos;
	return buf->writePos;
}

void artdaq::SharedMemoryManager::ResetReadPos(int buffer)
{
	TLOG(15) << "ResetReadPos(" << buffer << ") called.";

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "ResetReadPos obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "ResetReadPos obtained buffer_mutex for buffer " << buffer;

	//TraceLock lk(buffer_mutexes_[buffer], 18, "ResetReadPosBuffer" + std::to_string(buffer));
	auto buf = getBufferInfo_(buffer);
	if (!buf || buf->sem_id != manager_id_) return;
	touchBuffer_(buf);
	buf->readPos = 0;

	TLOG(15) << "ResetReadPos(" << buffer << ") ended.";
}

void artdaq::SharedMemoryManager::ResetWritePos(int buffer)
{
	TLOG(16) << "ResetWritePos(" << buffer << ") called.";

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "ResetWritePos obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "ResetWritePos obtained buffer_mutex for buffer " << buffer;

	//TraceLock lk(buffer_mutexes_[buffer], 18, "ResetWritePosBuffer" + std::to_string(buffer));
	auto buf = getBufferInfo_(buffer);
	if (!buf) return;
	checkBuffer_(buf, BufferSemaphoreFlags::Writing);
	touchBuffer_(buf);
	buf->writePos = 0;

	TLOG(16) << "ResetWritePos(" << buffer << ") ended.";
}

void artdaq::SharedMemoryManager::IncrementReadPos(int buffer, size_t read)
{
	TLOG(15) << "IncrementReadPos called: buffer= " << buffer << ", bytes to read=" << read;

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "IncrementReadPos obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "IncrementReadPos obtained buffer_mutex for buffer " << buffer;
	//TraceLock lk(buffer_mutexes_[buffer], 19, "IncReadPosBuffer" + std::to_string(buffer));
	auto buf = getBufferInfo_(buffer);
	if (!buf || buf->sem_id != manager_id_) return;
	touchBuffer_(buf);
	TLOG(15) << "IncrementReadPos: buffer= " << buffer << ", readPos=" << buf->readPos << ", bytes read=" << read;
	buf->readPos = buf->readPos + read;
	TLOG(15) << "IncrementReadPos: buffer= " << buffer << ", New readPos is " << buf->readPos;
	if (read == 0) Detach(true, "LogicError", "Cannot increment Read pos by 0! (buffer=" + std::to_string(buffer) + ", readPos=" + std::to_string(buf->readPos) + ", writePos=" + std::to_string(buf->writePos) + ")");
}

bool artdaq::SharedMemoryManager::IncrementWritePos(int buffer, size_t written)
{
	TLOG(16) << "IncrementWritePos called: buffer= " << buffer << ", bytes written=" << written;

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "IncrementWritePos obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "IncrementWritePos obtained buffer_mutex for buffer " << buffer;
	//TraceLock lk(buffer_mutexes_[buffer], 20, "IncWritePosBuffer" + std::to_string(buffer));
	auto buf = getBufferInfo_(buffer);
	if (!buf) return false;
	checkBuffer_(buf, BufferSemaphoreFlags::Writing);
	touchBuffer_(buf);
	if (buf->writePos + written > shm_ptr_->buffer_size)
	{
		TLOG(TLVL_ERROR) << "Requested write size is larger than the buffer size! (sz=" << std::hex << shm_ptr_->buffer_size << ", cur + req=" << std::dec << buf->writePos + written << ")";
		return false;
	}
	TLOG(16) << "IncrementWritePos: buffer= " << buffer << ", writePos=" << buf->writePos << ", bytes written=" << written;
	buf->writePos += written;
	TLOG(16) << "IncrementWritePos: buffer= " << buffer << ", New writePos is " << buf->writePos;
	if (written == 0) Detach(true, "LogicError", "Cannot increment Write pos by 0!");

	return true;
}

bool artdaq::SharedMemoryManager::MoreDataInBuffer(int buffer)
{
	TLOG(17) << "MoreDataInBuffer(" << buffer << ") called.";

	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "MoreDataInBuffer obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "MoreDataInBuffer obtained buffer_mutex for buffer " << buffer;
	//TraceLock lk(buffer_mutexes_[buffer], 21, "MoreDataInBuffer" + std::to_string(buffer));
	auto buf = getBufferInfo_(buffer);
	if (!buf) return false;
	TLOG(17) << "MoreDataInBuffer: buffer= " << buffer << ", readPos=" << std::to_string(buf->readPos) << ", writePos=" << buf->writePos;
	return buf->readPos < buf->writePos;
}

bool artdaq::SharedMemoryManager::CheckBuffer(int buffer, BufferSemaphoreFlags flags)
{
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "CheckBuffer obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "CheckBuffer obtained buffer_mutex for buffer " << buffer;
	//TraceLock lk(buffer_mutexes_[buffer], 22, "CheckBuffer" + std::to_string(buffer));
	return checkBuffer_(getBufferInfo_(buffer), flags, false);
}

void artdaq::SharedMemoryManager::MarkBufferFull(int buffer, int destination)
{
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "MarkBufferFull obtaining buffer_mutex for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "MarkBufferFull obtained buffer_mutex for buffer " << buffer;

	//TraceLock lk(buffer_mutexes_[buffer], 23, "FillBuffer" + std::to_string(buffer));
	auto shmBuf = getBufferInfo_(buffer);
	if (!shmBuf) return;
	touchBuffer_(shmBuf);
	if (shmBuf->sem_id == manager_id_)
	{
		if (shmBuf->sem != BufferSemaphoreFlags::Full)
			shmBuf->sem = BufferSemaphoreFlags::Full;

		shmBuf->sem_id = destination;
	}
}

void artdaq::SharedMemoryManager::MarkBufferEmpty(int buffer, bool force)
{
	TLOG(18) << "MarkBufferEmpty BEGIN, buffer=" << buffer << ", force=" << force << ", manager_id_=" << manager_id_;
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	//TraceLock lk(buffer_mutexes_[buffer], 24, "EmptyBuffer" + std::to_string(buffer));
	auto shmBuf = getBufferInfo_(buffer);
	if (!shmBuf) return;
	if (!force)
	{
		checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading, true);
	}
	touchBuffer_(shmBuf);

	shmBuf->readPos = 0;
	shmBuf->sem = BufferSemaphoreFlags::Full;

	if ((force && (manager_id_ == 0 || manager_id_ == shmBuf->sem_id)) || (!force && shm_ptr_->destructive_read_mode))
	{
		TLOG(18) << "MarkBufferEmpty Resetting buffer " << buffer << " to Empty state";
		shmBuf->writePos = 0;
		shmBuf->sem = BufferSemaphoreFlags::Empty;
		if (shm_ptr_->reader_pos == static_cast<unsigned>(buffer) && !shm_ptr_->destructive_read_mode)
		{
			TLOG(18) << "MarkBufferEmpty Broadcast mode; incrementing reader_pos from " << shm_ptr_->reader_pos << " to " << (buffer + 1) % shm_ptr_->buffer_count;
			shm_ptr_->reader_pos = (buffer + 1) % shm_ptr_->buffer_count;
		}
	}
	shmBuf->sem_id = -1;
	TLOG(18) << "MarkBufferEmpty END, buffer=" << buffer << ", force=" << force;
	;
}

bool artdaq::SharedMemoryManager::ResetBuffer(int buffer)
{
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");

	TLOG(TLVL_BUFLCK) << "ResetBuffer: obtaining buffer_mutex lock for buffer " << buffer;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	TLOG(TLVL_BUFLCK) << "ResetBuffer: obtained buffer_mutex lock for buffer " << buffer;

	//TraceLock lk(buffer_mutexes_[buffer], 25, "ResetBuffer" + std::to_string(buffer));
	auto shmBuf = getBufferInfo_(buffer);
	if (!shmBuf) return false;
	/*
		if (shmBuf->sequence_id < shm_ptr_->lowest_seq_id_read - size() && shmBuf->sem == BufferSemaphoreFlags::Full)
		{
			TLOG(TLVL_DEBUG) << "Buffer " << buffer << " has been passed by all readers, marking Empty" ;
			shmBuf->writePos = 0;
			shmBuf->sem = BufferSemaphoreFlags::Empty;
			shmBuf->sem_id = -1;
			return true;
		}*/

	size_t delta = TimeUtils::gettimeofday_us() - shmBuf->last_touch_time;
	if (delta > 0xFFFFFFFF)
	{
		TLOG(TLVL_TRACE) << "Buffer has touch time in the future, setting it to current time and ignoring...";
		shmBuf->last_touch_time = TimeUtils::gettimeofday_us();
		return false;
	}
	if (shm_ptr_->buffer_timeout_us == 0 || delta <= shm_ptr_->buffer_timeout_us || shmBuf->sem == BufferSemaphoreFlags::Empty) return false;
	TLOG(27) << "Buffer " << buffer << " at " << (void*)shmBuf << " is stale, time=" << TimeUtils::gettimeofday_us() << ", last touch=" << shmBuf->last_touch_time << ", d=" << delta << ", timeout=" << shm_ptr_->buffer_timeout_us;

	if (shmBuf->sem_id == manager_id_ && shmBuf->sem == BufferSemaphoreFlags::Writing)
	{
		return true;
	}

	if (!shm_ptr_->destructive_read_mode && shmBuf->sem == BufferSemaphoreFlags::Full && (shmBuf->sequence_id < last_seen_id_ || manager_id_ == 0))
	{
		TLOG(TLVL_DEBUG) << "Resetting old broadcast mode buffer " << buffer << " (seqid=" << shmBuf->sequence_id << "). State: Full-->Empty";
		shmBuf->writePos = 0;
		shmBuf->sem = BufferSemaphoreFlags::Empty;
		shmBuf->sem_id = -1;
		if (shm_ptr_->reader_pos == static_cast<unsigned>(buffer)) shm_ptr_->reader_pos = (buffer + 1) % shm_ptr_->buffer_count;
		return true;
	}

	if (shmBuf->sem_id != manager_id_ && shmBuf->sem == BufferSemaphoreFlags::Reading)
	{
		// Ron wants to re-check for potential interleave of buffer state updates
		size_t delta = TimeUtils::gettimeofday_us() - shmBuf->last_touch_time;
		if (delta <= shm_ptr_->buffer_timeout_us) return false;
		TLOG(TLVL_WARNING) << "Stale Read buffer " << buffer << " at " << (void*)shmBuf
		                   << " ( " << delta << " / " << shm_ptr_->buffer_timeout_us << " us ) detected! (seqid="
		                   << shmBuf->sequence_id << ") Resetting... Reading-->Full";
		shmBuf->readPos = 0;
		shmBuf->sem = BufferSemaphoreFlags::Full;
		shmBuf->sem_id = -1;
		return true;
	}
	return false;
}

bool artdaq::SharedMemoryManager::IsEndOfData() const
{
	if (!IsValid()) return true;

	struct shmid_ds info;
	auto sts = shmctl(shm_segment_id_, IPC_STAT, &info);
	if (sts < 0)
	{
		TLOG(TLVL_TRACE) << "Error accessing Shared Memory info: " << errno << " (" << strerror(errno) << ").";
		return true;
	}

	if (info.shm_perm.mode & SHM_DEST)
	{
		TLOG(TLVL_INFO) << "Shared Memory marked for destruction. Probably an end-of-data condition!";
		return true;
	}

	return false;
}

uint16_t artdaq::SharedMemoryManager::GetAttachedCount() const
{
	if (!IsValid()) return 0;

	struct shmid_ds info;
	auto sts = shmctl(shm_segment_id_, IPC_STAT, &info);
	if (sts < 0)
	{
		TLOG(TLVL_TRACE) << "Error accessing Shared Memory info: " << errno << " (" << strerror(errno) << ").";
		return 0;
	}

	return info.shm_nattch;
}

size_t artdaq::SharedMemoryManager::Write(int buffer, void* data, size_t size)
{
	TLOG(19) << "Write BEGIN";
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	//TraceLock lk(buffer_mutexes_[buffer], 26, "WriteBuffer" + std::to_string(buffer));
	auto shmBuf = getBufferInfo_(buffer);
	if (!shmBuf) return -1;
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Writing);
	touchBuffer_(shmBuf);
	TLOG(19) << "Buffer Write Pos is " << shmBuf->writePos << ", write size is " << size;
	if (shmBuf->writePos + size > shm_ptr_->buffer_size)
	{
		TLOG(TLVL_ERROR) << "Attempted to write more data than fits into Shared Memory, bufferSize=" << shm_ptr_->buffer_size
		                 << ",writePos=" << shmBuf->writePos << ",writeSize=" << size;
		Detach(true, "SharedMemoryWrite", "Attempted to write more data than fits into Shared Memory! \nRe-run with a larger buffer size!");
	}

	auto pos = GetWritePos(buffer);
	memcpy(pos, data, size);
	touchBuffer_(shmBuf);
	shmBuf->writePos = shmBuf->writePos + size;

	auto last_seen = last_seen_id_.load();
	while (last_seen < shmBuf->sequence_id && !last_seen_id_.compare_exchange_weak(last_seen, shmBuf->sequence_id)) {}

	TLOG(19) << "Write END";
	return size;
}

bool artdaq::SharedMemoryManager::Read(int buffer, void* data, size_t size)
{
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	//TraceLock lk(buffer_mutexes_[buffer], 27, "ReadBuffer" + std::to_string(buffer));
	auto shmBuf = getBufferInfo_(buffer);
	if (!shmBuf) return false;
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading);
	touchBuffer_(shmBuf);
	if (shmBuf->readPos + size > shm_ptr_->buffer_size)
	{
		TLOG(TLVL_ERROR) << "Attempted to read more data than fits into Shared Memory, bufferSize=" << shm_ptr_->buffer_size
		                 << ",readPos=" << shmBuf->readPos << ",readSize=" << size;
		Detach(true, "SharedMemoryRead", "Attempted to read more data than exists in Shared Memory!");
	}

	auto pos = GetReadPos(buffer);
	TLOG(TLVL_TRACE) << "Before memcpy in Read(), size is " << size;
	memcpy(data, pos, size);
	TLOG(TLVL_TRACE) << "After memcpy in Read()";
	auto sts = checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading, false);
	if (sts)
	{
		shmBuf->readPos += size;
		touchBuffer_(shmBuf);
		return true;
	}
	return false;
}

std::string artdaq::SharedMemoryManager::toString()
{
	std::ostringstream ostr;
	ostr << "ShmStruct: " << std::endl
	     << "Reader Position: " << shm_ptr_->reader_pos << std::endl
	     << "Writer Position: " << shm_ptr_->writer_pos << std::endl
	     << "Next ID Number: " << shm_ptr_->next_id << std::endl
	     << "Buffer Count: " << shm_ptr_->buffer_count << std::endl
	     << "Buffer Size: " << std::to_string(shm_ptr_->buffer_size) << " bytes" << std::endl
	     << "Buffers Written: " << std::to_string(shm_ptr_->next_sequence_id) << std::endl
	     << "Rank of Writer: " << shm_ptr_->rank << std::endl
	     << "Ready Magic Bytes: 0x" << std::hex << shm_ptr_->ready_magic << std::dec << std::endl
	     << std::endl;

	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buf = getBufferInfo_(ii);
		if (!buf) continue;

		ostr << "ShmBuffer " << std::dec << ii << std::endl
		     << "sequenceID: " << std::to_string(buf->sequence_id) << std::endl
		     << "writePos: " << std::to_string(buf->writePos) << std::endl
		     << "readPos: " << std::to_string(buf->readPos) << std::endl
		     << "sem: " << FlagToString(buf->sem) << std::endl
		     << "Owner: " << std::to_string(buf->sem_id.load()) << std::endl
		     << "Last Touch Time: " << std::to_string(buf->last_touch_time / 1000000.0) << std::endl
		     << std::endl;
	}

	return ostr.str();
}

void* artdaq::SharedMemoryManager::GetReadPos(int buffer)
{
	auto buf = getBufferInfo_(buffer);
	if (!buf) return nullptr;
	return bufferStart_(buffer) + buf->readPos;
}
void* artdaq::SharedMemoryManager::GetWritePos(int buffer)
{
	auto buf = getBufferInfo_(buffer);
	if (!buf) return nullptr;
	return bufferStart_(buffer) + buf->writePos;
}

void* artdaq::SharedMemoryManager::GetBufferStart(int buffer)
{
	return bufferStart_(buffer);
}

std::list<std::pair<int, artdaq::SharedMemoryManager::BufferSemaphoreFlags>> artdaq::SharedMemoryManager::GetBufferReport()
{
	auto output = std::list<std::pair<int, BufferSemaphoreFlags>>();
	for (size_t ii = 0; ii < size(); ++ii)
	{
		auto buf = getBufferInfo_(ii);
		output.emplace_back(std::make_pair(buf->sem_id.load(), buf->sem.load()));
	}
	return output;
}

uint8_t* artdaq::SharedMemoryManager::dataStart_() const
{
	if (shm_ptr_ == nullptr) return nullptr;
	return reinterpret_cast<uint8_t*>(shm_ptr_ + 1) + shm_ptr_->buffer_count * sizeof(ShmBuffer);
}

uint8_t* artdaq::SharedMemoryManager::bufferStart_(int buffer)
{
	if (shm_ptr_ == nullptr) return nullptr;
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");
	return dataStart_() + buffer * shm_ptr_->buffer_size;
}

artdaq::SharedMemoryManager::ShmBuffer* artdaq::SharedMemoryManager::getBufferInfo_(int buffer)
{
	if (shm_ptr_ == nullptr) return nullptr;
	if (buffer >= shm_ptr_->buffer_count) Detach(true, "ArgumentOutOfRange", "The specified buffer does not exist!");
	return reinterpret_cast<ShmBuffer*>(reinterpret_cast<uint8_t*>(shm_ptr_ + 1) + buffer * sizeof(ShmBuffer));
}

bool artdaq::SharedMemoryManager::checkBuffer_(ShmBuffer* buffer, BufferSemaphoreFlags flags, bool exceptions)
{
	if (!buffer)
	{
		if (exceptions)
		{
			Detach(true, "BufferNotThereException", "Request to check buffer that does not exist!");
		}
		return false;
	}
	TLOG(TLVL_TRACE) << "checkBuffer_: Checking that buffer " << buffer->sequence_id << " has sem_id " << manager_id_ << " (Current: " << buffer->sem_id << ") and is in state " << FlagToString(flags) << " (current: " << FlagToString(buffer->sem) << ")";
	if (exceptions)
	{
		if (buffer->sem != flags) Detach(true, "StateAccessViolation", "Shared Memory buffer is not in the correct state! (expected " + FlagToString(flags) + ", actual " + FlagToString(buffer->sem) + ")");
		if (buffer->sem_id != manager_id_) Detach(true, "OwnerAccessViolation", "Shared Memory buffer is not owned by this manager instance! (Expected: " + std::to_string(manager_id_) + ", Actual: " + std::to_string(buffer->sem_id) + ")");
	}
	bool ret = (buffer->sem_id == manager_id_ || (buffer->sem_id == -1 && (flags == BufferSemaphoreFlags::Full || flags == BufferSemaphoreFlags::Empty))) && buffer->sem == flags;

	if (!ret)
	{
		TLOG(TLVL_WARNING) << "CheckBuffer detected issue with buffer " << buffer->sequence_id << "!"
		                   << " ID: " << buffer->sem_id << " (Expected " << manager_id_ << "), Flag: " << FlagToString(buffer->sem) << " (Expected " << FlagToString(flags) << "). "
		                   << "ID -1 is okay if expected flag is \"Full\" or \"Empty\".";
	}

	return ret;
}

void artdaq::SharedMemoryManager::touchBuffer_(ShmBuffer* buffer)
{
	if (!buffer || (buffer->sem_id != -1 && buffer->sem_id != manager_id_)) return;
	TLOG(TLVL_TRACE) << "touchBuffer_: Touching buffer at " << (void*)buffer << " with sequence_id " << buffer->sequence_id;
	buffer->last_touch_time = TimeUtils::gettimeofday_us();
}

void artdaq::SharedMemoryManager::Detach(bool throwException, std::string category, std::string message, bool force)
{
	TLOG(TLVL_DETACH) << "Detach BEGIN: throwException: " << std::boolalpha << throwException << ", force: " << force;
	if (IsValid())
	{
		TLOG(TLVL_DETACH) << "Detach: Resetting owned buffers";
		auto bufs = GetBuffersOwnedByManager(false);
		for (auto buf : bufs)
		{
			auto shmBuf = getBufferInfo_(buf);
			if (!shmBuf) continue;
			if (shmBuf->sem == BufferSemaphoreFlags::Writing)
			{
				shmBuf->sem = BufferSemaphoreFlags::Empty;
			}
			else if (shmBuf->sem == BufferSemaphoreFlags::Reading)
			{
				shmBuf->sem = BufferSemaphoreFlags::Full;
			}
			shmBuf->sem_id = -1;
		}
	}

	if (shm_ptr_)
	{
		TLOG(TLVL_DETACH) << "Detach: Detaching shared memory";
		shmdt(shm_ptr_);
		shm_ptr_ = NULL;
	}

	if ((force || manager_id_ == 0) && shm_segment_id_ > -1)
	{
		TLOG(TLVL_DETACH) << "Detach: Marking Shared memory for removal";
		shmctl(shm_segment_id_, IPC_RMID, NULL);
		shm_segment_id_ = -1;
	}

	// Reset manager_id_
	manager_id_ = -1;

	if (category.size() > 0 && message.size() > 0)
	{
		TLOG(TLVL_ERROR) << category << ": " << message;

		if (throwException)
		{
			throw cet::exception(category) << message;
		}
	}
}

// Local Variables:
// mode: c++
// End:
