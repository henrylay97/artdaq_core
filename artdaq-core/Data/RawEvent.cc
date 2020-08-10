#include "artdaq-core/Data/RawEvent.hh"
#include <ostream>

namespace artdaq {
void detail::RawEventHeader::print(std::ostream& os) const
{
	os << "Run " << run_id
	   << ", Subrun " << subrun_id
	   << ", Event " << event_id
	   << ", SeqID " << sequence_id
	   << ", TS " << timestamp
	   << ", Complete? " << is_complete
	   << ", Version " << static_cast<unsigned int>(version)
	   << '\n';
}

constexpr uint8_t detail::RawEventHeader::CURRENT_VERSION;
void RawEvent::print(std::ostream& os) const
{
	os << "Run " << runID()
	   << ", Subrun " << subrunID()
	   << ", Event " << eventID()
	   << ", SeqID " << sequenceID()
	   << ", TS " << timestamp()
	   << ", FragCount " << numFragments()
	   << ", WordCount " << wordCount()
	   << ", Complete? " << isComplete()
	   << '\n';
	for (auto const& frag : fragments_)
	{
		os << *frag << '\n';
	}
}
}  // namespace artdaq
