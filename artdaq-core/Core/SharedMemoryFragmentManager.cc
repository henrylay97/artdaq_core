
#define TRACE_NAME "SharedMemoryFragmentManager"
#include "artdaq-core/Core/SharedMemoryFragmentManager.hh"
#include "tracemf.h"

artdaq::SharedMemoryFragmentManager::SharedMemoryFragmentManager(uint32_t shm_key, size_t buffer_count, size_t max_buffer_size, size_t buffer_timeout_us)
	: SharedMemoryManager(shm_key, buffer_count, max_buffer_size, buffer_timeout_us)
	, active_buffer_(-1)
{
}

bool artdaq::SharedMemoryFragmentManager::ReadyForWrite(bool overwrite)
{
	TLOG(12) << "ReadyForWrite: active_buffer is " << active_buffer_;
	if (active_buffer_ != -1) return true;
	active_buffer_ = GetBufferForWriting(overwrite);

	return active_buffer_ != -1;
}

int artdaq::SharedMemoryFragmentManager::WriteFragment(Fragment&& fragment, bool overwrite, size_t timeout_us)
{
	if (!IsValid() || IsEndOfData())
	{
		TLOG(TLVL_WARNING) << "WriteFragment: Shared memory is not connected! Attempting reconnect...";
		auto sts = Attach(timeout_us);
		if (!sts)
		{
			return -1;
		}
		TLOG(TLVL_INFO) << "WriteFragment: Shared memory was successfully reconnected";
	}

	auto waitStart = std::chrono::steady_clock::now();
	while (!ReadyForWrite(overwrite) && TimeUtils::GetElapsedTimeMicroseconds(waitStart) < 1000)
	{
		// BURN THAT CPU!
	}
	if (!ReadyForWrite(overwrite))
	{
		int64_t loopCount = 0;
		size_t sleepTime = 1000; // microseconds
		int64_t nloops = (timeout_us - 1000) / sleepTime;

		while (!ReadyForWrite(overwrite) && (!overwrite || timeout_us == 0 || loopCount < nloops))
		{
			usleep(sleepTime);
			++loopCount;
		}
	}
	if (!ReadyForWrite(overwrite))
	{
		TLOG(TLVL_WARNING) << "No available buffers after waiting for " << TimeUtils::GetElapsedTimeMicroseconds(waitStart) << " us.";
		return -3;
	}

	TLOG(13) << "Sending fragment with seqID=" << fragment.sequenceID() << " using buffer " << active_buffer_;
	artdaq::RawDataType* fragAddr = fragment.headerAddress();
	size_t fragSize = fragment.size() * sizeof(artdaq::RawDataType);

	auto sts = Write(active_buffer_, fragAddr, fragSize);
	if (sts == fragSize)
	{
		TLOG(13) << "Done sending Fragment with seqID=" << fragment.sequenceID() << " using buffer " << active_buffer_;
		MarkBufferFull(active_buffer_);
		active_buffer_ = -1;
		return 0;
	}
	active_buffer_ = -1;
	TLOG(TLVL_ERROR) << "Unexpected status from SharedMemory Write call!";
	return -2;
}

// NOT currently (2018-07-22) used! ReadFragmentHeader and ReadFragmentData
// (below) are called directly
int artdaq::SharedMemoryFragmentManager::ReadFragment(Fragment& fragment)
{
	TLOG(14) << "ReadFragment BEGIN";
	detail::RawFragmentHeader tmpHdr;

	TLOG(14) << "Reading Fragment Header";
	auto sts = ReadFragmentHeader(tmpHdr);
	if (sts != 0) return sts;
	fragment.resize(tmpHdr.word_count - tmpHdr.num_words());
	memcpy(fragment.headerAddress(), &tmpHdr, tmpHdr.num_words() * sizeof(artdaq::RawDataType));
	TLOG(14) << "Reading Fragment Body - of frag w/ seqID="<<tmpHdr.sequence_id;
	return ReadFragmentData(fragment.headerAddress() + tmpHdr.num_words(), tmpHdr.word_count - tmpHdr.num_words());
}

int artdaq::SharedMemoryFragmentManager::ReadFragmentHeader(detail::RawFragmentHeader& header)
{
	if (!IsValid())
	{
		TLOG(22) << "ReadFragmentHeader: !IsValid(), returning -3";
		return -3;
	}

	size_t hdrSize = artdaq::detail::RawFragmentHeader::num_words() * sizeof(artdaq::RawDataType);
	active_buffer_ = GetBufferForReading();

	if (active_buffer_ == -1)
	{
		TLOG(22) << "ReadFragmentHeader: active_buffer==-1, returning -1";
		return -1;
	}

	auto sts = Read(active_buffer_, &header, hdrSize);
	if (!sts)
	{
		TLOG(TLVL_ERROR) << "ReadFragmentHeader: Buffer " << active_buffer_ << " returned bad status code from Read";
		MarkBufferEmpty(active_buffer_);
		active_buffer_ = -1;
		return -2;
	}

	TLOG(22) << "ReadFragmentHeader: read active_buffer_="<<active_buffer_<<" sequence_id="<<header.sequence_id;
	return 0;
}

int artdaq::SharedMemoryFragmentManager::ReadFragmentData(RawDataType* destination, size_t words)
{
	if (!IsValid() || active_buffer_ == -1 || !CheckBuffer(active_buffer_, BufferSemaphoreFlags::Reading))
	{
		TLOG(TLVL_ERROR) << "ReadFragmentData: Buffer " << active_buffer_ << " failed status checks: IsValid()=" << std::boolalpha << IsValid() << ", CheckBuffer=" << CheckBuffer(active_buffer_, BufferSemaphoreFlags::Reading);
		return -3;
	}

	auto sts = Read(active_buffer_, destination, words * sizeof(RawDataType));
	if (!sts)
	{
		TLOG(TLVL_ERROR) << "ReadFragmentData: Buffer " << active_buffer_ << " returned bad status code from Read";
		MarkBufferEmpty(active_buffer_);
		active_buffer_ = -1;
		return -2;
	}

	MarkBufferEmpty(active_buffer_);
	active_buffer_ = -1;
	return 0;
}
