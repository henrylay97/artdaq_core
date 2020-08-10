#ifndef artdaq_core_Data_RawEvent_hh
#define artdaq_core_Data_RawEvent_hh

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/dictionarycontrol.hh"

#include "cetlib_except/exception.h"

#include <algorithm>
#include <iosfwd>
#include <memory>

namespace artdaq {
/**
	 * \brief artdaq implementation details namespace
	 */
namespace detail {
struct RawEventHeader;
}

/**
	 * \brief The header information used to identify key properties of the RawEvent object
	 * 
	 * RawEventHeader is the artdaq generic event header. It contains
	 * the information necessary for routing of raw events inside the
	 * artdaq code, but is not intended for use by any experiment.
	 */
struct detail::RawEventHeader
{
	static constexpr uint8_t CURRENT_VERSION = 0;
	typedef uint32_t run_id_t;       ///< Run numbers are 32 bits
	typedef uint32_t subrun_id_t;    ///< Subrun numbers are 32 bits
	typedef uint32_t event_id_t;     ///< Event numbers are 32 bits
	typedef uint64_t sequence_id_t;  ///< Field size should be the same as the Fragment::sequence_id field
	typedef uint64_t timestamp_t;    ///< Field size should be the same as the Fragment::timestamp field

	run_id_t run_id;            ///< Fragments don't know about runs
	subrun_id_t subrun_id;      ///< Fragments don't know about subruns
	event_id_t event_id;        ///< Event number should be either sequence ID or Timestamp of component Fragments
	sequence_id_t sequence_id;  ///< RawEvent sequence_id should be the same as its component Fragment sequence_ids.
	timestamp_t timestamp;      ///< The timestamp of the first Fragment received for this event
	bool is_complete;           ///< Does the event contain the expected number of Fragment objects?
	uint8_t version;            ///< Version number of the RawFragmentHeader

	/**
		 * \brief Default constructor. Provided for ROOT compatibility
		 */
	RawEventHeader()
	    : run_id(0), subrun_id(0), event_id(0), sequence_id(0), timestamp(0), is_complete(false), version(CURRENT_VERSION) {}

	/**
		 * \brief Constructs the RawEventHeader struct with the given parameters
		 * \param run The current Run number
		 * \param subrun The current Subrun number
		 * \param event The current event number
		 * \param seq The current Sequence ID
		 * \param ts The current Timestamp
		 */
	RawEventHeader(run_id_t run,
	               subrun_id_t subrun,
	               event_id_t event,
	               sequence_id_t seq,
	               timestamp_t ts)
	    : run_id(run)
	    , subrun_id(subrun)
	    , event_id(event)
	    , sequence_id(seq)
	    , timestamp(ts)
	    , is_complete(false)
	    , version(CURRENT_VERSION)
	{}

	void print(std::ostream& os) const;
};

#if HIDE_FROM_ROOT
/**
	 * \brief Prints the RawEventHeader to the given stream
	 * \param os Stream to print RawEventHeader to
	 * \param evh RawEventHeader to print
	 * \return Stream reference
	 */
inline std::ostream& operator<<(std::ostream& os, detail::RawEventHeader const& evh)
{
	evh.print(os);
	return os;
}
#endif

/**
	 * \brief RawEvent is the artdaq view of a generic event, containing a header and zero or more Fragments.
	 * 
	 * RawEvent should be a class, not a struct; it should be enforcing
	 * invariants (the contained Fragments should all have the correct
	 * event id).
	 */
class RawEvent
{
public:
	typedef detail::RawEventHeader::run_id_t run_id_t;            ///< Run numbers are 32 bits
	typedef detail::RawEventHeader::subrun_id_t subrun_id_t;      ///< Subrun numbers are 32 bits
	typedef detail::RawEventHeader::event_id_t event_id_t;        ///< Event numbers are 32 bits
	typedef detail::RawEventHeader::sequence_id_t sequence_id_t;  ///< Field size should be the same as the Fragment::sequence_id field
	typedef detail::RawEventHeader::timestamp_t timestamp_t;      ///< Field size should be the same as the Fragment::timestamp field

	/**
		 * \brief Constructs a RawEvent with the given parameters
		 * \param run The current Run number
		 * \param subrun The current Subrun number
		 * \param event The current Event number
		 * \param seq The current sequence_id
		 * \param ts The timestamp for the event
		 */
	RawEvent(run_id_t run, subrun_id_t subrun, event_id_t event, sequence_id_t seq, timestamp_t ts);

	/**
		 * \brief Constructs a RawEvent using the given RawEventHeader
		 * \param hdr Header to use for initializing RawEvent
		 */
	explicit RawEvent(detail::RawEventHeader hdr);

#if HIDE_FROM_ROOT
	/**
		 * \brief Insert the given (pointer to a) Fragment into this RawEvent.
		 * \param pfrag The FragmentPtr to insert into the RawEvent
		 * \exception cet::exception if pfrag is nullptr
		 * 
		 * Insert the given (pointer to a) Fragment into this
		 * RawEvent. This takes ownership of the Fragment referenced by
		 * the FragmentPtr, unless an exception is thrown.
		 */
	void insertFragment(FragmentPtr&& pfrag);

	/**
		 * \brief Mark the event as complete
		 */
	void markComplete();

	/**
		 * \brief Return the number of fragments this RawEvent contains.
		 * \return The number of Fragment objects in this RawEvent
		 */
	size_t numFragments() const;

	/**
		 * \brief Return the sum of the word counts of all fragments in this RawEvent.
		 * \return The sum of the word counts of all Fragment objects in this RawEvent
		 */
	size_t wordCount() const;

	/**
		 * \brief Retrieve the run number from the RawEventHeader
		 * \return The run number stored in the RawEventHeader
		 */
	run_id_t runID() const;

	/**
		 * \brief Retrieve the subrun number from the RawEventHeader
		 * \return The subrun number stored in the RawEventHeader
		 */
	subrun_id_t subrunID() const;

	/**
		 * \brief Retrieve the event number from the RawEventHeader
		 * \return The event number stored in the RawEventHeader
		 */
	event_id_t eventID() const;

	/**
		 * \brief Retrieve the sequence id from the RawEventHeader
		 * \return The sequence id stored in the RawEventHeader
		 */
	sequence_id_t sequenceID() const;

	/**
	 * @brief Retrieve the timestamp from the RawEventHeader
	 * @return The timestamp stored in the RawEventHeader
	*/
	timestamp_t timestamp() const;

	/**
		 * \brief Retrieve the value of the complete flag from the RawEventHeader
		 * \return The value of RawEventHeader::is_complete
		 */
	bool isComplete() const;

	/**
		 * \brief Print summary information about this RawEvent to the given stream.
		 * \param os The target stream for summary information
		 */
	void print(std::ostream& os) const;

	/**
		 * \brief Release all the Fragments from this RawEvent
		 * \return A pointer to a Fragments object (owns the Fragment data contained)
		 * 
		 * Release all the Fragments from this RawEvent, returning them to
		 * the caller through a unique_ptr that manages a vector into which
		 * the Fragments have been moved.
		 */
	std::unique_ptr<Fragments> releaseProduct();

	/**
		 * \brief Fills in a list of unique fragment types from this event
		 * \param type_list Any Fragment types not included in this list will be added
		 */
	void fragmentTypes(std::vector<Fragment::type_t>& type_list);

	/**
		 * \brief Release Fragments from the RawEvent
		 * \param type The type of Fragments to release
		 * \return A pointer to a Fragments object (owns the Fragment data contained)
		 * 
		* Release the Fragments from this RawEvent with the specified
		* fragment type, returning them to the caller through a unique_ptr
		* that manages a vector into which the Fragments have been moved.
		* PLEASE NOTE that releaseProduct and releaseProduct(type_t) can not
		* both be used on the same RawEvent since each one gives up
		* ownership of the fragments within the event.
		 */
	std::unique_ptr<Fragments> releaseProduct(Fragment::type_t type);

#endif

private:
	detail::RawEventHeader header_;
	FragmentPtrs fragments_;
};

typedef std::shared_ptr<RawEvent> RawEvent_ptr;  ///< A shared_ptr to a RawEvent

inline RawEvent::RawEvent(run_id_t run, subrun_id_t subrun, event_id_t event, sequence_id_t seq, timestamp_t ts)
    : header_(run, subrun, event, seq, ts)
    , fragments_() {}

inline RawEvent::RawEvent(detail::RawEventHeader hdr)
    : header_(hdr), fragments_()
{}

#if HIDE_FROM_ROOT
inline void RawEvent::insertFragment(FragmentPtr&& pfrag)
{
	if (pfrag == nullptr)
	{
		throw cet::exception("LogicError")  // NOLINT(cert-err60-cpp)
		    << "Attempt to insert a null FragmentPtr into a RawEvent detected.\n";
	}
	fragments_.emplace_back(std::move(pfrag));
}

inline void RawEvent::markComplete() { header_.is_complete = true; }

inline size_t RawEvent::numFragments() const
{
	return fragments_.size();
}

inline size_t RawEvent::wordCount() const
{
	size_t sum = 0;
	for (auto const& frag : fragments_) { sum += frag->size(); }
	return sum;
}

inline RawEvent::run_id_t RawEvent::runID() const { return header_.run_id; }
inline RawEvent::subrun_id_t RawEvent::subrunID() const { return header_.subrun_id; }
inline RawEvent::event_id_t RawEvent::eventID() const { return header_.event_id; }
inline RawEvent::sequence_id_t RawEvent::sequenceID() const { return header_.sequence_id; }
inline RawEvent::timestamp_t RawEvent::timestamp() const { return header_.timestamp; }
inline bool RawEvent::isComplete() const { return header_.is_complete; }

inline std::unique_ptr<Fragments> RawEvent::releaseProduct()
{
	std::unique_ptr<Fragments> result(new Fragments);
	result->reserve(fragments_.size());
	// 03/08/2016 ELF: Moving to range-for for STL compatibility
	//for (size_t i = 0, sz = fragments_.size(); i < sz; ++i) {
	for (auto& i : fragments_)
	{
		result->emplace_back(std::move(*i));
	}
	// It seems more hygenic to clear fragments_ rather than to leave
	// it full of unique_ptrs to Fragments that have been plundered by
	// the move.
	fragments_.clear();
	return result;
}

inline void RawEvent::fragmentTypes(std::vector<Fragment::type_t>& type_list)
{
	// 03/08/2016 ELF: Moving to range-for for STL compatibility
	//for (size_t i = 0, sz = fragments_.size(); i < sz; ++i) {
	for (auto& i : fragments_)
	{
		auto fragType = i->type();  // fragments_[i]->type();
		if (std::find(type_list.begin(), type_list.end(), fragType) == type_list.end())
		{
			type_list.push_back(fragType);
		}
	}
	//std::sort(type_list.begin(), type_list.end());
	//std::unique(type_list.begin(), type_list.end());
}

inline std::unique_ptr<Fragments>
RawEvent::releaseProduct(Fragment::type_t fragment_type)
{
	std::unique_ptr<Fragments> result(new Fragments);
	auto iter = fragments_.begin();
	do
	{
		if ((*iter)->type() == fragment_type)
		{
			result->push_back(std::move(*(*iter)));
			iter = fragments_.erase(iter);
		}
		else
		{
			++iter;
		}
	} while (iter != fragments_.end());
	return result;
}

/**
	 * \brief Prints the RawEvent to the given stream
	 * \param os Stream to print RawEvent to
	 * \param ev RawEvent to print
	 * \return Stream reference
	 */
inline std::ostream& operator<<(std::ostream& os, RawEvent const& ev)
{
	ev.print(os);
	return os;
}

#endif
}  // namespace artdaq

#endif /* artdaq_core_Data_RawEvent_hh */
