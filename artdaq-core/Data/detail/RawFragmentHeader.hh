#ifndef artdaq_core_Data_detail_RawFragmentHeader_hh
#define artdaq_core_Data_detail_RawFragmentHeader_hh
// detail::RawFragmentHeader is an overlay that provides the user's view
// of the data contained within a Fragment. It is intended to be hidden
// from the user of Fragment, as an implementation detail. The interface
// of Fragment is intended to be used to access the data.

//#include <cstddef>
#include <map>
#include "artdaq-core/Data/dictionarycontrol.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "cetlib_except/exception.h"

extern "C" {
#include <stdint.h>
}

namespace artdaq {
namespace detail {
struct RawFragmentHeader;
}
}  // namespace artdaq

/**
 * \brief The RawFragmentHeader class contains the basic fields used by _artdaq_ for routing Fragment objects through the system.
 *
 * The RawFragmentHeader class contains the basic fields used by _artdaq_ for routing Fragment objects through the system. It also
 * contains static value definitions of values used in those fields.
 *
 */
struct artdaq::detail::RawFragmentHeader
{
	/**
	 * \brief The RawDataType (currently a 64-bit integer) is the basic unit of data representation within _artdaq_
	 */
	typedef unsigned long long RawDataType;

#if HIDE_FROM_ROOT
	typedef uint16_t version_t;             ///< version field is 16 bits
	typedef uint64_t sequence_id_t;         ///< sequence_id field is 48 bits
	typedef uint8_t type_t;                 ///< type field is 8 bits
	typedef uint16_t fragment_id_t;         ///< fragment_id field is 16 bits
	typedef uint8_t metadata_word_count_t;  ///< metadata_word_count field is 8 bits
	typedef uint64_t timestamp_t;           ///< timestamp field is 32 bits

	// define special values for type_t
	static constexpr type_t INVALID_TYPE = 0;                                 ///< Marks a Fragment as Invalid
	static constexpr type_t FIRST_USER_TYPE = 1;                              ///< The first user-accessible type
	static constexpr type_t LAST_USER_TYPE = 224;                             ///< The last user-accessible type (types above this number are system types
	static constexpr type_t FIRST_SYSTEM_TYPE = 225;                          ///< The first system type
	static constexpr type_t LAST_SYSTEM_TYPE = 255;                           ///< The last system type
	static constexpr type_t InvalidFragmentType = INVALID_TYPE;               ///< Marks a Fragment as Invalid
	static constexpr type_t EndOfDataFragmentType = FIRST_SYSTEM_TYPE;        ///< This Fragment indicates the end of data to _art_
	static constexpr type_t DataFragmentType = FIRST_SYSTEM_TYPE + 1;         ///< This Fragment holds data. Used for RawEvent Fragments sent from the EventBuilder to the Aggregator
	static constexpr type_t InitFragmentType = FIRST_SYSTEM_TYPE + 2;         ///< This Fragment holds the necessary data for initializing _art_
	static constexpr type_t EndOfRunFragmentType = FIRST_SYSTEM_TYPE + 3;     ///< This Fragment indicates the end of a run to _art_
	static constexpr type_t EndOfSubrunFragmentType = FIRST_SYSTEM_TYPE + 4;  ///< This Fragment indicates the end of a subrun to _art_
	static constexpr type_t ShutdownFragmentType = FIRST_SYSTEM_TYPE + 5;     ///< This Fragment indicates a system shutdown to _art_
	static constexpr type_t EmptyFragmentType = FIRST_SYSTEM_TYPE + 6;        ///< This Fragment contains no data and serves as a placeholder for when no data from a FragmentGenerator is expected
	static constexpr type_t ContainerFragmentType = FIRST_SYSTEM_TYPE + 7;    ///< This Fragment is a ContainerFragment and analysis code should unpack it
	static constexpr type_t ErrorFragmentType = FIRST_SYSTEM_TYPE + 8;        ///< This Fragment has experienced some error, and no attempt should be made to read it

	/**
	 * \brief Returns a map of the most-commonly used system types
	 * \return A map of the system types used in the _artdaq_ data stream
	 */
	static std::map<type_t, std::string> MakeSystemTypeMap()
	{
		return std::map<type_t, std::string>{
		    {type_t(DataFragmentType), "Data"},
		    {type_t(EmptyFragmentType), "Empty"},
		    {type_t(ErrorFragmentType), "Error"},
		    {type_t(InvalidFragmentType), "Invalid"},
		    {232, "Container"}};
	}

	/**
	 * \brief Returns a map of all system types
	 * \return A map of all defined system types
	 */
	static std::map<type_t, std::string> MakeVerboseSystemTypeMap()
	{
		return std::map<type_t, std::string>{
		    {type_t(EndOfDataFragmentType), "EndOfData"},
		    {type_t(DataFragmentType), "Data"},
		    {type_t(InitFragmentType), "Init"},
		    {type_t(EndOfRunFragmentType), "EndOfRun"},
		    {type_t(EndOfSubrunFragmentType), "EndOfSubrun"},
		    {type_t(ShutdownFragmentType), "Shutdown"},
		    {type_t(EmptyFragmentType), "Empty"},
		    {type_t(ContainerFragmentType), "Container"}};
	}

	/**
	 * \brief Print a system type's string name
	 * \param type Type to print
	 * \return String with "Name" of type
	 */
	static std::string SystemTypeToString(type_t type)
	{
		switch (type)
		{
			case INVALID_TYPE:
				return "INVALID";
			case EndOfDataFragmentType:
				return "EndOfData";
			case DataFragmentType:
				return "Data";
			case InitFragmentType:
				return "Init";
			case EndOfRunFragmentType:
				return "EndOfRun";
			case EndOfSubrunFragmentType:
				return "EndOfSubrun";
			case ShutdownFragmentType:
				return "Shutdown";
			case EmptyFragmentType:
				return "Empty";
			case ContainerFragmentType:
				return "Container";
		}
		return "Unknown";
	}

	// Each of the following invalid values is chosen based on the
	// size of the bitfield in which the corresponding data are
	// encoded; if any of the sizes are changed, the corresponding
	// values must be updated.
	static const version_t InvalidVersion = 0xFFFF;                  ///< The version field is currently 16-bits.
	static const version_t CurrentVersion = 0x2;                     ///< The CurrentVersion field should be incremented whenever the RawFragmentHeader changes
	static const sequence_id_t InvalidSequenceID = 0xFFFFFFFFFFFF;   ///< The sequence_id field is currently 48-bits
	static const fragment_id_t InvalidFragmentID = 0xFFFF;           ///< The fragment_id field is currently 16-bits
	static const timestamp_t InvalidTimestamp = 0xFFFFFFFFFFFFFFFF;  ///< The timestamp field is currently 64-bits

	RawDataType word_count : 32;          ///< number of RawDataType words in this Fragment
	RawDataType version : 16;             ///< The version of the fragment.
	RawDataType type : 8;                 ///< The type of the fragment, either system or user-defined
	RawDataType metadata_word_count : 8;  ///< The number of RawDataType words in the user-defined metadata

	RawDataType sequence_id : 48;  ///< The 48-bit sequence_id uniquely identifies events within the _artdaq_ system
	RawDataType fragment_id : 16;  ///< The fragment_id uniquely identifies a particular piece of hardware within the _artdaq_ system

	RawDataType timestamp : 64;  ///< The 64-bit timestamp field is the output of a user-defined clock used for building time-correlated events

	RawDataType valid : 1;      ///< Flag for whether the Fragment has been transported correctly through the artdaq system
	RawDataType complete : 1;   ///< Flag for whether the Fragment completely represents an event for its hardware
	RawDataType atime_ns : 30;  ///< Last access time of the Fragment, nanosecond part
	RawDataType atime_s : 32;   ///< Last access time of the Fragment, second part (measured from epoch)

	// ****************************************************
	// New fields MUST be added to the END of this list!!!
	// ****************************************************

	/**
	 * \brief Returns the number of RawDataType words present in the header
	 * \return The number of RawDataType words present in the header
	 */
	static constexpr std::size_t num_words();

	/**
	 * \brief Sets the type field to the specified user type
	 * \param utype The type code to set
	 * \exception cet::exception if utype is not in the allowed range for user types
	 */
	void setUserType(uint8_t utype);

	/**
	* \brief Sets the type field to the specified system type
	* \param stype The type code to set
	* \exception cet::exception if stype is not in the allowed range for system types
	*/
	void setSystemType(uint8_t stype);

	/**
	 * \brief Update the atime fields of the RawFragmentHeader to current time
	 */
	void touch();
	/**
	 * \brief Get the last access time of this RawFragmentHeader
	 * \return struct timespec representing last access time of this RawFragmentHeader
	 */
	struct timespec atime();
	/**
	 * \brief Get the elapsed time between now and the last access time of the RawFragmentHeader, optionally resetting it
	 * \param touch Whether to also update the access time to current time
	 * \return struct timespec representing interval between now and last access time of this RawFragmentHeader
	 */
	struct timespec getLatency(bool touch);

#endif /* HIDE_FROM_ROOT */
};

#if HIDE_FROM_ROOT
inline constexpr std::size_t
artdaq::detail::RawFragmentHeader::num_words()
{
	return sizeof(detail::RawFragmentHeader) / sizeof(RawDataType);
}

// Compile-time check that the assumption made in num_words() above is
// actually true.
static_assert((artdaq::detail::RawFragmentHeader::num_words() *
               sizeof(artdaq::detail::RawFragmentHeader::RawDataType)) ==
                  sizeof(artdaq::detail::RawFragmentHeader),
              "sizeof(RawFragmentHeader) is not an integer "
              "multiple of sizeof(RawDataType)!");

inline void
artdaq::detail::RawFragmentHeader::setUserType(uint8_t utype)
{
	if (utype < FIRST_USER_TYPE || utype > LAST_USER_TYPE)
	{
		throw cet::exception("InvalidValue")
		    << "RawFragmentHeader user types must be in the range of "
		    << ((int)FIRST_USER_TYPE) << " to " << ((int)LAST_USER_TYPE)
		    << " (bad type is " << ((int)utype) << ").";
	}
	type = utype;
}

inline void
artdaq::detail::RawFragmentHeader::setSystemType(uint8_t stype)
{
	if (stype < FIRST_SYSTEM_TYPE /*|| stype > LAST_SYSTEM_TYPE*/)
	{
		throw cet::exception("InvalidValue")
		    << "RawFragmentHeader system types must be in the range of "
		    << ((int)FIRST_SYSTEM_TYPE) << " to " << ((int)LAST_SYSTEM_TYPE);
	}
	type = stype;
}

inline void artdaq::detail::RawFragmentHeader::touch()
{
	auto time = artdaq::TimeUtils::get_realtime_clock();
	atime_ns = time.tv_nsec;
	atime_s = time.tv_sec;
}

inline struct timespec artdaq::detail::RawFragmentHeader::atime()
{
	struct timespec ts;
	ts.tv_nsec = atime_ns;
	ts.tv_sec = atime_s;
	return ts;
}

inline struct timespec artdaq::detail::RawFragmentHeader::getLatency(bool touch)
{
	auto a_time = atime();
	auto time = artdaq::TimeUtils::get_realtime_clock();

	a_time.tv_sec = time.tv_sec - a_time.tv_sec;

	if (a_time.tv_nsec > time.tv_nsec)
	{
		a_time.tv_sec--;
		a_time.tv_nsec = 1000000000 + time.tv_nsec - a_time.tv_nsec;
	}
	else
	{
		a_time.tv_nsec = time.tv_nsec - a_time.tv_nsec;
	}

	if (touch)
	{
		atime_ns = time.tv_nsec;
		atime_s = time.tv_sec;
	}
	return a_time;
}
#endif

#endif /* artdaq_core_Data_detail_RawFragmentHeader_hh */
