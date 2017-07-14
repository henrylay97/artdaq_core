#include <sys/shm.h>
#include "cetlib_except/exception.h"
#include "artdaq-core/Core/SharedMemoryManager.hh"
#include "tracemf.h"

artdaq::SharedMemoryManager::SharedMemoryManager(uint32_t shm_key, size_t buffer_count, size_t max_buffer_size, uint64_t buffer_timeout_us)
	: shm_segment_id_(-1)
	, shm_ptr_(NULL)
	, shm_key_(shm_key)
	, manager_id_(-1)
	, buffer_timeout_us_(buffer_timeout_us)
	, buffer_mutexes_()
{
	size_t shmSize = buffer_count * (max_buffer_size + sizeof(ShmBuffer)) + sizeof(ShmStruct);

	shm_segment_id_ = shmget(shm_key_, shmSize, 0666);

	if (shm_segment_id_ == -1)
	{
		TLOG_DEBUG("SharedMemoryManager") << "Creating shared memory segment with key 0x" << std::hex << std::to_string(shm_key_) << TLOG_ENDL;
		shm_segment_id_ = shmget(shm_key_, shmSize, IPC_CREAT | 0666);
		manager_id_ = 0;
	}

	TLOG_DEBUG("SharedMemoryManager") << "shm_key == 0x" << std::hex << std::to_string(shm_key_) << ", shm_segment_id == " << shm_segment_id_ << TLOG_ENDL;

	if (shm_segment_id_ > -1)
	{
		TLOG_DEBUG("SharedMemoryManager")
			<< "Attached to shared memory segment with ID = " << shm_segment_id_
			<< " and size " << shmSize
			<< " bytes" << TLOG_ENDL;
		shm_ptr_ = (ShmStruct*)shmat(shm_segment_id_, 0, 0);
		TLOG_DEBUG("SharedMemoryManager")
			<< "Attached to shared memory segment at address "
			<< std::hex << shm_ptr_ << std::dec << TLOG_ENDL;
		if (shm_ptr_ && shm_ptr_ != (void *)-1)
		{
			if (manager_id_ == 0)
			{
				shm_ptr_->next_id = 1;
				shm_ptr_->reader_pos = 0;
				shm_ptr_->writer_pos = 0;
				shm_ptr_->buffer_size = max_buffer_size;
				shm_ptr_->buffer_count = buffer_count;

				for (size_t ii = 0; ii < buffer_count; ++ii)
				{
					getBufferInfo_(ii)->writePos = 0;
					getBufferInfo_(ii)->readPos = 0;
					getBufferInfo_(ii)->sem = BufferSemaphoreFlags::Empty;
					getBufferInfo_(ii)->sem_id = -1;
				}
				shm_ptr_->ready_magic = 0xCAFE1111;
			}
			else
			{
				while (shm_ptr_->ready_magic != 0xCAFE1111) { usleep(1000); }
				GetNewId();
			}
		}
		else
		{
			TLOG_ERROR("SharedMemoryManager") << "Failed to attach to shared memory segment "
				<< shm_segment_id_ << TLOG_ENDL;
		}
	}
	else
	{
		TLOG_ERROR("SharedMemoryManager") << "Failed to connect to shared memory segment"
			<< ", errno = " << errno << ".  Please check "
			<< "if a stale shared memory segment needs to "
			<< "be cleaned up. (ipcs, ipcrm -m <segId>)" << TLOG_ENDL;
	}
}

artdaq::SharedMemoryManager::~SharedMemoryManager()
{
	TLOG_ARB(5, "SharedMemoryManager") << "~SharedMemoryManager called" << TLOG_ENDL;
	if (shm_ptr_)
	{
		shmdt(shm_ptr_);
		shm_ptr_ = NULL;
	}

	if (manager_id_ == 0 && shm_segment_id_ > -1)
	{
		shmctl(shm_segment_id_, IPC_RMID, NULL);
	}
	TLOG_ARB(5, "SharedMemoryManager") << "~SharedMemoryManager done" << TLOG_ENDL;
}

int artdaq::SharedMemoryManager::GetBufferForReading()
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	auto rp = shm_ptr_->reader_pos.load();
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (ii + rp) % shm_ptr_->buffer_count;

		ResetBuffer(buffer);

		auto buf = getBufferInfo_(buffer);
		if (buf->sem == BufferSemaphoreFlags::Full && (buf->sem_id == -1 || buf->sem_id == manager_id_))
		{
			buf->sem_id = manager_id_;
			buf->sem = BufferSemaphoreFlags::Reading;
			if (buf->sem_id != manager_id_) continue;
			buf->readPos = 0;
			shm_ptr_->reader_pos = (buffer + 1) % shm_ptr_->buffer_count;
			return buffer;
		}
	}

	return -1;
}

int artdaq::SharedMemoryManager::GetBufferForWriting(bool overwrite)
{
	TLOG_ARB(13, "SharedMemoryManager") << "GetBufferForWriting BEGIN" << TLOG_ENDL;
	std::unique_lock<std::mutex> lk(search_mutex_);
	auto wp = shm_ptr_->writer_pos.load();
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (ii + wp) % shm_ptr_->buffer_count;

		ResetBuffer(buffer);

		auto buf = getBufferInfo_(buffer);
		if (buf->sem == BufferSemaphoreFlags::Empty || (overwrite && buf->sem != BufferSemaphoreFlags::Writing))
		{
			buf->sem_id = manager_id_;
			buf->sem = BufferSemaphoreFlags::Writing;
			if (buf->sem_id != manager_id_) continue;
			buf->writePos = 0;
			shm_ptr_->writer_pos = (buffer + 1) % shm_ptr_->buffer_count;
			TLOG_ARB(13, "SharedMemoryManager") << "GetBufferForWriting returning " << buffer << TLOG_ENDL;
			return buffer;
		}
	}

	TLOG_ARB(13, "SharedMemoryManager") << "GetBufferForWriting Returning -1 because no buffers are ready" << TLOG_ENDL;
	return -1;
}

bool artdaq::SharedMemoryManager::ReadyForRead() const
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	auto rp = shm_ptr_->reader_pos.load();
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (ii + rp) % shm_ptr_->buffer_count;
		auto buf = getBufferInfo_(buffer);
		if (buf->sem == BufferSemaphoreFlags::Full && (buf->sem_id == -1 || buf->sem_id == manager_id_))
		{
			TLOG_ARB(13, "SharedMemoryManager") << "ReadyForRead returning true because buffer " << ii << " is ready." << TLOG_ENDL;
			return true;
		}
	}

	//TLOG_ARB(13, "SharedMemoryManager") << "ReadyForRead returning false" << TLOG_ENDL;
	return false;

}

size_t artdaq::SharedMemoryManager::ReadReadyCount() const
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	size_t count = 0;
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buf = getBufferInfo_(ii);
		if (buf->sem == BufferSemaphoreFlags::Full && (buf->sem_id == -1 || buf->sem_id == manager_id_))
		{
			++count;
		}
	}
	return count;
}

bool artdaq::SharedMemoryManager::ReadyForWrite(bool overwrite) const
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	auto wp = shm_ptr_->writer_pos.load();
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buffer = (ii + wp) % shm_ptr_->buffer_count;
		auto buf = getBufferInfo_(buffer);
		if ((buf->sem == BufferSemaphoreFlags::Empty && buf->sem_id == -1)
			|| (overwrite && buf->sem != BufferSemaphoreFlags::Writing))
		{
			TLOG_ARB(13, "SharedMemoryManager") << "ReadyForWrite returning true because buffer " << ii << " is ready." << TLOG_ENDL;
			return true;
		}
	}

	//TLOG_ARB(13, "SharedMemoryManager") << "ReadyForWrite returning false" << TLOG_ENDL;
	return false;

}

size_t artdaq::SharedMemoryManager::WriteReadyCount(bool overwrite) const
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	size_t count = 0;
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buf = getBufferInfo_(ii);
		if ((buf->sem == BufferSemaphoreFlags::Empty && buf->sem_id == -1)
			|| (overwrite && buf->sem != BufferSemaphoreFlags::Writing))
		{
			++count;
		}
	}
	return count;
}

std::deque<int> artdaq::SharedMemoryManager::GetBuffersOwnedByManager()
{
	std::unique_lock<std::mutex> lk(search_mutex_);
	std::deque<int> output;
	for (auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buf = getBufferInfo_(ii);
		if (buf->sem_id == manager_id_) {
			touchBuffer_(buf);
			output.push_back(ii);
		}
	}

	return output;
}

size_t artdaq::SharedMemoryManager::BufferDataSize(int buffer)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	return buf->writePos;
}


void artdaq::SharedMemoryManager::ResetReadPos(int buffer)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	buf->readPos = 0;
}

void artdaq::SharedMemoryManager::IncrementReadPos(int buffer, size_t read)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	TLOG_ARB(13, "SharedMemoryManager") << "IncrementReadPos: buffer= " << buffer << ", readPos=" << std::to_string(buf->readPos) << ", bytes read=" << std::to_string(read) << TLOG_ENDL;
	buf->readPos = buf->readPos + read;
	TLOG_ARB(13, "SharedMemoryManager") << "IncrementReadPos: buffer= " << buffer << ", New readPos is " << std::to_string(buf->readPos) << TLOG_ENDL;
	if (read == 0)
		throw cet::exception("LogicError") << "Cannot increment Read pos by 0! (buffer=" << std::to_string(buffer) << ", readPos=" << std::to_string(buf->readPos) << ")";
}

void artdaq::SharedMemoryManager::IncrementWritePos(int buffer, size_t written)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	TLOG_ARB(13, "SharedMemoryManager") << "IncrementWritePos: buffer= " << buffer << ", writePos=" << std::to_string(buf->writePos) << ", bytes written=" << std::to_string(written) << TLOG_ENDL;
	buf->writePos += written;
	TLOG_ARB(13, "SharedMemoryManager") << "IncrementWritePos: buffer= " << buffer << ", New writePos is " << std::to_string(buf->writePos) << TLOG_ENDL;
	if (written == 0) throw cet::exception("LogicError") << "Cannot increment Write pos by 0!";
}

bool artdaq::SharedMemoryManager::MoreDataInBuffer(int buffer)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	TLOG_ARB(13, "SharedMemoryManager") << "MoreDataInBuffer: buffer= " << buffer << ", readPos=" << std::to_string(buf->readPos) << ", writePos=" << std::to_string(buf->writePos) << TLOG_ENDL;
	return buf->readPos < buf->writePos;
}

bool artdaq::SharedMemoryManager::CheckBuffer(int buffer, BufferSemaphoreFlags flags)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	return checkBuffer_(getBufferInfo_(buffer), flags, false);
}

void artdaq::SharedMemoryManager::MarkBufferFull(int buffer, int destination)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto shmBuf = getBufferInfo_(buffer);
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Writing);
	touchBuffer_(shmBuf);

	shmBuf->sem_id = destination;
	shmBuf->sem = BufferSemaphoreFlags::Full;
}

void artdaq::SharedMemoryManager::MarkBufferEmpty(int buffer)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto shmBuf = getBufferInfo_(buffer);
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading);
	touchBuffer_(shmBuf);

	shmBuf->readPos = 0;
	shmBuf->writePos = 0;
	shmBuf->sem_id = -1;
	shmBuf->sem = BufferSemaphoreFlags::Empty;
}

void artdaq::SharedMemoryManager::ResetBuffer(int buffer)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto shmBuf = getBufferInfo_(buffer);
	if (shmBuf->sem_id != manager_id_ && shmBuf->buffer_touch_time > TimeUtils::gettimeofday_us() - buffer_timeout_us_) return;

	if (shmBuf->sem == BufferSemaphoreFlags::Reading)
	{
		shmBuf->readPos = 0;
		shmBuf->sem = BufferSemaphoreFlags::Full;
		shmBuf->sem_id = -1;
	}
	else if (shmBuf->sem == BufferSemaphoreFlags::Writing)
	{
		shmBuf->writePos = 0;
		shmBuf->sem = BufferSemaphoreFlags::Empty;
		shmBuf->sem_id = -1;
	}
}

size_t artdaq::SharedMemoryManager::Write(int buffer, void* data, size_t size)
{
	TLOG_ARB(13, "SharedMemoryManager") << "Write BEGIN" << TLOG_ENDL;
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto shmBuf = getBufferInfo_(buffer);
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Writing);
	touchBuffer_(shmBuf);
	TLOG_ARB(13, "SharedMemoryManager") << "Buffer Write Pos is " << std::to_string(shmBuf->writePos) << ", write size is " << std::to_string(size) << TLOG_ENDL;
	if (shmBuf->writePos + size > shm_ptr_->buffer_size) throw cet::exception("SharedMemoryWrite") << "Attempted to write more data than fits into Shared Memory!"
		<< std::endl << "Re-run with a larger buffer size!";

	auto pos = GetWritePos(buffer);
	memcpy(pos, data, size);
	shmBuf->writePos = shmBuf->writePos + size;
	TLOG_ARB(13, "SharedMemoryManager") << "Write END" << TLOG_ENDL;
	return size;
}

bool artdaq::SharedMemoryManager::Read(int buffer, void* data, size_t size)
{
	std::unique_lock<std::mutex> lk(buffer_mutexes_[buffer]);
	auto shmBuf = getBufferInfo_(buffer);
	checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading);
	touchBuffer_(shmBuf);
	if (shmBuf->readPos + size > shm_ptr_->buffer_size) throw cet::exception("SharedMemoryRead") << "Attempted to read more data than exists in Shared Memory!";

	auto pos = GetReadPos(buffer);
	memcpy(data, pos, size);
	shmBuf->readPos += size;
	return checkBuffer_(shmBuf, BufferSemaphoreFlags::Reading, false);
}

std::string artdaq::SharedMemoryManager::toString()
{
	std::ostringstream ostr;
	ostr << "ShmStruct: " << std::endl
		<< "Buffer Count: " << shm_ptr_->buffer_count << std::endl
		<< "Buffer Size: " << std::to_string(shm_ptr_->buffer_size) << " bytes" << std::endl
		<< "Rank of Writer: " << shm_ptr_->rank << std::endl
		<< "Ready Magic Bytes: " << std::to_string(shm_ptr_->ready_magic) << std::endl << std::endl;

	for(auto ii = 0; ii < shm_ptr_->buffer_count; ++ii)
	{
		auto buf = getBufferInfo_(ii);
		ostr << "ShmBuffer " << ii << std::endl
			<< "writePos: " << std::to_string(buf->writePos) << std::endl
			<< "readPos: " << std::to_string(buf->readPos) << std::endl
			<< "sem: " << FlagToString(buf->sem) << std::endl
			<< "Owner: " << std::to_string(buf->sem_id.load()) << std::endl
			<< "Last Touch Time: " << std::to_string(buf->buffer_touch_time) << std::endl << std::endl;
	}
	return ostr.str();
}

void* artdaq::SharedMemoryManager::GetReadPos(int buffer)
{
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	return bufferStart_(buffer) + buf->readPos;
}
void* artdaq::SharedMemoryManager::GetWritePos(int buffer)
{
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	return bufferStart_(buffer) + buf->writePos;
}

void* artdaq::SharedMemoryManager::GetBufferStart(int buffer)
{
	auto buf = getBufferInfo_(buffer);
	touchBuffer_(buf);
	return bufferStart_(buffer);
}

uint8_t* artdaq::SharedMemoryManager::dataStart_() const
{
	return reinterpret_cast<uint8_t*>(shm_ptr_ + 1) + shm_ptr_->buffer_count * sizeof(ShmBuffer);
}

uint8_t* artdaq::SharedMemoryManager::bufferStart_(int buffer) const
{
	if (buffer >= shm_ptr_->buffer_count) throw cet::exception("ArgumentOutOfRange") << "The specified buffer does not exist!";
	return dataStart_() + buffer * shm_ptr_->buffer_size;
}

artdaq::SharedMemoryManager::ShmBuffer* artdaq::SharedMemoryManager::getBufferInfo_(int buffer) const
{
	if (buffer >= shm_ptr_->buffer_count) throw cet::exception("ArgumentOutOfRange") << "The specified buffer does not exist!";
	auto buf = reinterpret_cast<ShmBuffer*>(reinterpret_cast<uint8_t*>(shm_ptr_ + 1) + buffer * sizeof(ShmBuffer));
	return buf;
}

bool artdaq::SharedMemoryManager::checkBuffer_(ShmBuffer* buffer, BufferSemaphoreFlags flags, bool exceptions)
{
	if (exceptions)
	{
		if (buffer->sem != flags) throw cet::exception("StateAccessViolation") << "Shared Memory buffer is not in the correct state! (expected " << FlagToString(flags) << ", actual " << FlagToString(buffer->sem) << ")";
		if (buffer->sem_id != manager_id_) throw cet::exception("OwnerAccessViolation") << "Shared Memory buffer is not owned by this manager instance!";
	}
	return buffer->sem_id == manager_id_ && buffer->sem == flags;
}

void artdaq::SharedMemoryManager::touchBuffer_(ShmBuffer* buffer)
{
	buffer->buffer_touch_time = TimeUtils::gettimeofday_us();
}



// Local Variables:
// mode: c++
// End:
