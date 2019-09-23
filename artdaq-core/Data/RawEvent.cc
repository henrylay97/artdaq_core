#include "artdaq-core/Data/RawEvent.hh"
#include <ostream>

namespace artdaq {
void RawEvent::print(std::ostream& os) const
{
	os << "Run " << runID()
	   << ", Subrun " << subrunID()
	   << ", Event " << eventID()
	   << ", SeqID " << sequenceID()
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
