
#include "artdaq-core/Core/SharedMemoryEventReceiver.hh"

#include "artdaq-core/Data/Fragment.hh"
#include <sys/time.h>
#define TRACE_NAME "SharedMemoryEventReceiver"
#include "tracemf.h"

using std::string;

artdaq::SharedMemoryEventReceiver::SharedMemoryEventReceiver(uint32_t shm_key, uint32_t broadcast_shm_key)
	: current_read_buffer_(-1)
	, initialized_(false)
	, current_header_(nullptr)
	, current_data_source_(nullptr)
	, data_(shm_key)
	, broadcasts_(broadcast_shm_key)
{
	TLOG(TLVL_TRACE) << "SharedMemoryEventReceiver CONSTRUCTOR" ;
}

bool artdaq::SharedMemoryEventReceiver::ReadyForRead(bool broadcast, size_t timeout_us)
{
	TLOG(TLVL_TRACE) << "ReadyForRead BEGIN timeout_us=" << timeout_us ;
	if (current_read_buffer_ != -1 && current_data_source_ && current_header_)
	{
		TLOG(TLVL_TRACE) << "ReadyForRead Returning true because already reading buffer" ;
		return true;
	}

	bool first = true;
	auto start_time = TimeUtils::gettimeofday_us();
	uint64_t time_diff = 0;
	int buf = -1;
	while (first || time_diff < timeout_us)
	{
		if (broadcasts_.ReadyForRead())
		{
			buf = broadcasts_.GetBufferForReading();
			current_data_source_ = &broadcasts_;
		}
		else if (!broadcast && data_.ReadyForRead())
		{
			buf = data_.GetBufferForReading();
			current_data_source_ = &data_;
		}
		if (buf != -1 && current_data_source_)
		{
			current_read_buffer_ = buf;
			current_data_source_->ResetReadPos(buf);
			current_header_ = reinterpret_cast<detail::RawEventHeader*>(current_data_source_->GetReadPos(buf));
			TLOG(TLVL_TRACE) << "ReadyForRead Found buffer, returning true. event hdr sequence_id=" << current_header_->sequence_id;

			// Ignore any Init fragments after the first
			if (current_data_source_ == &broadcasts_)
			{
				bool err;
				auto types = GetFragmentTypes(err);
				if (!err && types.count(Fragment::type_t(Fragment::InitFragmentType)) && initialized_)
				{
					ReleaseBuffer();
					continue;
				}
				else if (!err && types.count(Fragment::type_t(Fragment::InitFragmentType)))
				{
					initialized_ = true;
				}
			}

			return true;
		}
		current_data_source_ = nullptr;
		first = false;

		time_diff = TimeUtils::gettimeofday_us() - start_time;
		usleep(time_diff < 10000 ? time_diff : 10000 );
	}
	TLOG(TLVL_TRACE) << "ReadyForRead returning false" ;
	return false;
}

artdaq::detail::RawEventHeader* artdaq::SharedMemoryEventReceiver::ReadHeader(bool& err)
{
	TLOG(TLVL_TRACE) << "ReadHeader BEGIN" ;
	if (current_read_buffer_ != -1 && current_data_source_)
	{
		err = !current_data_source_->CheckBuffer(current_read_buffer_, SharedMemoryManager::BufferSemaphoreFlags::Reading);
		if (err)
		{
			TLOG(TLVL_WARNING) << "Buffer was in incorrect state, resetting" ;
			current_data_source_ = nullptr;
			current_read_buffer_ = -1;
			current_header_ = nullptr;
			return nullptr;
		}
	}
	TLOG(TLVL_TRACE) << "Already have buffer, returning stored header" ;
	return current_header_;
}

std::set<artdaq::Fragment::type_t> artdaq::SharedMemoryEventReceiver::GetFragmentTypes(bool& err)
{
	if (current_read_buffer_ == -1 || !current_header_ || !current_data_source_) throw cet::exception("AccessViolation") << "Cannot call GetFragmentTypes when not currently reading a buffer! Call ReadHeader() first!";

	err = !current_data_source_->CheckBuffer(current_read_buffer_, SharedMemoryManager::BufferSemaphoreFlags::Reading);
	if (err) return std::set<Fragment::type_t>();

	current_data_source_->ResetReadPos(current_read_buffer_);
	current_data_source_->IncrementReadPos(current_read_buffer_, sizeof(detail::RawEventHeader));
	auto output = std::set<Fragment::type_t>();

	while (current_data_source_->MoreDataInBuffer(current_read_buffer_))
	{
		err = !current_data_source_->CheckBuffer(current_read_buffer_, SharedMemoryManager::BufferSemaphoreFlags::Reading);
		if (err) return std::set<Fragment::type_t>();
		auto fragHdr = reinterpret_cast<artdaq::detail::RawFragmentHeader*>(current_data_source_->GetReadPos(current_read_buffer_));
		output.insert(fragHdr->type);
		current_data_source_->IncrementReadPos(current_read_buffer_, fragHdr->word_count * sizeof(RawDataType));
	}

	return output;
}


std::unique_ptr<artdaq::Fragments> artdaq::SharedMemoryEventReceiver::GetFragmentsByType(bool& err, Fragment::type_t type)
{
	if (!current_data_source_ || !current_header_ || current_read_buffer_ == -1) throw cet::exception("AccessViolation") << "Cannot call GetFragmentsByType when not currently reading a buffer! Call ReadHeader() first!";
	err = !current_data_source_->CheckBuffer(current_read_buffer_, SharedMemoryManager::BufferSemaphoreFlags::Reading);
	if (err) return nullptr;

	current_data_source_->ResetReadPos(current_read_buffer_);
	current_data_source_->IncrementReadPos(current_read_buffer_, sizeof(detail::RawEventHeader));

	std::unique_ptr<Fragments> output(new Fragments());

	while (current_data_source_->MoreDataInBuffer(current_read_buffer_))
	{
		err = !current_data_source_->CheckBuffer(current_read_buffer_, SharedMemoryManager::BufferSemaphoreFlags::Reading);
		if (err) return nullptr;
		auto fragHdr = reinterpret_cast<artdaq::detail::RawFragmentHeader*>(current_data_source_->GetReadPos(current_read_buffer_));
		if (fragHdr->type == type || type == Fragment::InvalidFragmentType)
		{
			output->emplace_back(fragHdr->word_count - detail::RawFragmentHeader::num_words());
			current_data_source_->Read(current_read_buffer_, output->back().headerAddress(), fragHdr->word_count * sizeof(RawDataType));
			output->back().autoResize();
		}
		else
		{
			current_data_source_->IncrementReadPos(current_read_buffer_, fragHdr->word_count * sizeof(RawDataType));
		}
	}

	return output;
}

std::string artdaq::SharedMemoryEventReceiver::printBuffers_(SharedMemoryManager* data_source)
{
	std::ostringstream ostr;
	for (size_t ii = 0; ii < data_source->size(); ++ii)
	{
		ostr << "Buffer " << ii << ": " << std::endl;

		data_source->ResetReadPos(ii);
		data_source->IncrementReadPos(ii, sizeof(detail::RawEventHeader));

		while (data_source->MoreDataInBuffer(ii))
		{
			auto fragHdr = reinterpret_cast<artdaq::detail::RawFragmentHeader*>(data_source->GetReadPos(ii));
			ostr << "    Fragment " << fragHdr->fragment_id << ": Sequence ID: " << fragHdr->sequence_id << ", Type:" << fragHdr->type;
			if (fragHdr->MakeVerboseSystemTypeMap().count(fragHdr->type))
			{
				ostr << " (" << fragHdr->MakeVerboseSystemTypeMap()[fragHdr->type] << ")";
			}
			ostr << ", Size: " << fragHdr->word_count << " words." << std::endl;
			data_source->IncrementReadPos(ii, fragHdr->word_count * sizeof(RawDataType));
		}

	}
	return ostr.str();
}

std::string artdaq::SharedMemoryEventReceiver::toString()
{
	std::ostringstream ostr;
	ostr << data_.toString() << std::endl;

	ostr << "Data Buffer Fragment Counts: " << std::endl;
	ostr << printBuffers_(&data_);

	if (data_.GetKey() != broadcasts_.GetKey())
	{
		ostr << "Broadcast Buffer Fragment Counts: " << std::endl;
		ostr << printBuffers_(&broadcasts_);
	}

	return ostr.str();
}

void artdaq::SharedMemoryEventReceiver::ReleaseBuffer()
{
	TLOG(TLVL_TRACE) << "ReleaseBuffer BEGIN" ;
	try
	{
		if(current_data_source_) current_data_source_->MarkBufferEmpty(current_read_buffer_);
	}
	catch (cet::exception const& e)
	{
		TLOG(TLVL_WARNING) << "A cet::exception occured while trying to release the buffer: " << e ;
	}
	catch (...)
	{
		TLOG(TLVL_ERROR) << "An unknown exception occured while trying to release the buffer" ;
	}
	current_read_buffer_ = -1;
	current_header_ = nullptr;
	current_data_source_ = nullptr;
	TLOG(TLVL_TRACE) << "ReleaseBuffer END" ;
}
