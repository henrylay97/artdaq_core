#include "artdaq-core/Data/Fragment.hh"

#include <cmath>
#include <iostream>

using artdaq::detail::RawFragmentHeader;

bool artdaq::fragmentSequenceIDCompare(const Fragment& i, const Fragment& j)
{
	return i.sequenceID() < j.sequenceID();
}

artdaq::Fragment::Fragment()
    : vals_(RawFragmentHeader::num_words(), -1)
{
	fragmentHeaderPtr()->version = RawFragmentHeader::CurrentVersion;
	updateFragmentHeaderWC_();
	fragmentHeaderPtr()->type = InvalidFragmentType;
	fragmentHeaderPtr()->metadata_word_count = 0;
	fragmentHeaderPtr()->touch();
}

artdaq::Fragment::Fragment(std::size_t n)
    : vals_(n + RawFragmentHeader::num_words())
{
	// vals ctor w/o init val is used; make sure header is ALL initialized.
	for (iterator ii = vals_.begin();
	     ii != (vals_.begin() + RawFragmentHeader::num_words()); ++ii)
	{
		*ii = -1;
	}
	fragmentHeaderPtr()->version = RawFragmentHeader::CurrentVersion;
	updateFragmentHeaderWC_();
	fragmentHeaderPtr()->type = Fragment::InvalidFragmentType;
	fragmentHeaderPtr()->sequence_id = Fragment::InvalidSequenceID;
	fragmentHeaderPtr()->fragment_id = Fragment::InvalidFragmentID;
	fragmentHeaderPtr()->timestamp = Fragment::InvalidTimestamp;
	fragmentHeaderPtr()->metadata_word_count = 0;
	fragmentHeaderPtr()->touch();
}

artdaq::Fragment::Fragment(sequence_id_t sequenceID,
                           fragment_id_t fragID,
                           type_t type,
                           timestamp_t timestamp)
    : vals_(RawFragmentHeader::num_words(), -1)
{
	fragmentHeaderPtr()->version = RawFragmentHeader::CurrentVersion;
	updateFragmentHeaderWC_();
	if (type == Fragment::DataFragmentType)
	{
		// this value is special because it is the default specified
		// in the constructor declaration
		fragmentHeaderPtr()->setSystemType(type);
	}
	else
	{
		fragmentHeaderPtr()->setUserType(type);
	}
	fragmentHeaderPtr()->sequence_id = sequenceID;
	fragmentHeaderPtr()->fragment_id = fragID;
	fragmentHeaderPtr()->timestamp = timestamp;
	fragmentHeaderPtr()->metadata_word_count = 0;
	fragmentHeaderPtr()->touch();
}

#if HIDE_FROM_ROOT
void artdaq::Fragment::print(std::ostream& os) const
{
  os << "Fragment:  " << fragmentID() << '\n'
     << "WordCount: " << size() << '\n'
     << "Event:     " << sequenceID() << '\n'
     << "Timestamp: " << timestamp() << '\n';
}

artdaq::FragmentPtr
artdaq::Fragment::eodFrag(size_t nFragsToExpect)
{
	artdaq::FragmentPtr result(new Fragment(static_cast<size_t>(ceil(sizeof(nFragsToExpect) /
	                                                                 static_cast<double>(sizeof(value_type))))));
	result->setSystemType(Fragment::EndOfDataFragmentType);
	*result->dataBegin() = nFragsToExpect;
	return result;
}

artdaq::FragmentPtr
artdaq::Fragment::
    dataFrag(sequence_id_t sequenceID,
             fragment_id_t fragID,
             RawDataType const* dataPtr,
             size_t dataSize,
             timestamp_t timestamp)
{
	FragmentPtr result(new Fragment(sequenceID, fragID, Fragment::DataFragmentType, timestamp));
	result->resize(dataSize);
	memcpy(result->dataAddress(), dataPtr, (dataSize * sizeof(RawDataType)));
	return result;
}
#endif
