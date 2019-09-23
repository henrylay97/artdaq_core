#ifndef artdaq_core_Data_detail_RawFragmentHeaderV0_hh
#define artdaq_core_Data_detail_RawFragmentHeaderV0_hh
// detail::RawFragmentHeaderV0 is an overlay that provides the user's view
// of the data contained within a Fragment. It is intended to be hidden
// from the user of Fragment, as an implementation detail. The interface
// of Fragment is intended to be used to access the data.

//#include <cstddef>
#include <map>
#include "artdaq-core/Data/detail/RawFragmentHeader.hh"
#include "artdaq-core/Data/dictionarycontrol.hh"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "cetlib_except/exception.h"

extern "C" {
#include <stdint.h>
}

namespace artdaq {
namespace detail {
struct RawFragmentHeaderV0;
}
}  // namespace artdaq

/**
 * \brief The RawFragmentHeaderV0 class contains the basic fields used by _artdaq_ for routing Fragment objects through the system.
 *
 * The RawFragmentHeaderV0 class contains the basic fields used by _artdaq_ for routing Fragment objects through the system. It also
 * contains static value definitions of values used in those fields.
 * This is an old version of RawFragmentHeader, provided for compatibility
 *
 */
struct artdaq::detail::RawFragmentHeaderV0
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
	typedef uint32_t timestamp_t;           ///< timestamp field is 32 bits

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

	/**
	 * \brief Returns a map of the most-commonly used system types
	 * \return A map of the system types used in the _artdaq_ data stream
	 */
	static std::map<type_t, std::string> MakeSystemTypeMap()
	{
		return std::map<type_t, std::string>{
		    {226, "Data"},
		    {231, "Empty"},
		    {232, "Container"}};
	}

	/**
	 * \brief Returns a map of all system types
	 * \return A map of all defined system types
	 */
	static std::map<type_t, std::string> MakeVerboseSystemTypeMap()
	{
		return std::map<type_t, std::string>{
		    {225, "EndOfData"},
		    {226, "Data"},
		    {227, "Init"},
		    {228, "EndOfRun"},
		    {229, "EndOfSubrun"},
		    {230, "Shutdown"},
		    {231, "Empty"},
		    {232, "Container"}};
	}

	// Each of the following invalid values is chosen based on the
	// size of the bitfield in which the corresponding data are
	// encoded; if any of the sizes are changed, the corresponding
	// values must be updated.
	static const version_t InvalidVersion = 0xFFFF;                 ///< The version field is currently 16-bits.
	static const version_t CurrentVersion = 0x0;                    ///< The CurrentVersion field should be incremented whenever the RawFragmentHeader changes
	static const sequence_id_t InvalidSequenceID = 0xFFFFFFFFFFFF;  ///< The sequence_id field is currently 48-bits
	static const fragment_id_t InvalidFragmentID = 0xFFFF;          ///< The fragment_id field is currently 16-bits
	static const timestamp_t InvalidTimestamp = 0xFFFFFFFF;         ///< The timestamp field is currently 32-bits

	RawDataType word_count : 32;          ///< number of RawDataType words in this Fragment
	RawDataType version : 16;             ///< The version of the fragment. Currently always InvalidVersion
	RawDataType type : 8;                 ///< The type of the fragment, either system or user-defined
	RawDataType metadata_word_count : 8;  ///< The number of RawDataType words in the user-defined metadata

	RawDataType sequence_id : 48;  ///< The 48-bit sequence_id uniquely identifies events within the _artdaq_ system
	RawDataType fragment_id : 16;  ///< The fragment_id uniquely identifies a particular piece of hardware within the _artdaq_ system
	RawDataType timestamp : 32;    ///< The 64-bit timestamp field is the output of a user-defined clock used for building time-correlated events

	RawDataType unused1 : 16;  ///< Extra space
	RawDataType unused2 : 16;  ///< Extra space

	/**
	 * \brief Returns the number of RawDataType words present in the header
	 * \return The number of RawDataType words present in the header
	 */
	constexpr static std::size_t num_words();

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
	 * \brief Upgrades the RawFragmentHeaderV0 to a RawFragmentHeader (Current version)
	 * \return Current-version RawFragmentHeader
	 * 
	 * The constraints on RawFragmentHeader upgrades are that no field may shrink in size
	 * or be deleted. Therefore, there will always be an upgrade path from old RawFragmentHeaders
	 * to new ones. By convention, all fields are initialized to the Invalid defines, and then
	 * the old data (guarenteed to be smaller) is cast to the new header. In the case of added
	 * fields, they will remain marked Invalid.
	 */
	RawFragmentHeader upgrade() const;

#endif /* HIDE_FROM_ROOT */
};

#if HIDE_FROM_ROOT
inline constexpr std::size_t
artdaq::detail::RawFragmentHeaderV0::num_words()
{
	return sizeof(detail::RawFragmentHeaderV0) / sizeof(RawDataType);
}

// Compile-time check that the assumption made in num_words() above is
// actually true.
static_assert((artdaq::detail::RawFragmentHeaderV0::num_words() *
               sizeof(artdaq::detail::RawFragmentHeaderV0::RawDataType)) ==
                  sizeof(artdaq::detail::RawFragmentHeaderV0),
              "sizeof(RawFragmentHeader) is not an integer "
              "multiple of sizeof(RawDataType)!");

inline void
artdaq::detail::RawFragmentHeaderV0::setUserType(uint8_t utype)
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
artdaq::detail::RawFragmentHeaderV0::setSystemType(uint8_t stype)
{
	if (stype < FIRST_SYSTEM_TYPE /*|| stype > LAST_SYSTEM_TYPE*/)
	{
		throw cet::exception("InvalidValue")
		    << "RawFragmentHeader system types must be in the range of "
		    << ((int)FIRST_SYSTEM_TYPE) << " to " << ((int)LAST_SYSTEM_TYPE);
	}
	type = stype;
}

inline artdaq::detail::RawFragmentHeader
artdaq::detail::RawFragmentHeaderV0::upgrade() const
{
	RawFragmentHeader output;
	output.word_count = word_count;
	output.version = RawFragmentHeader::CurrentVersion;
	output.type = type;
	output.metadata_word_count = metadata_word_count;

	output.sequence_id = sequence_id;
	output.fragment_id = fragment_id;
	output.timestamp = timestamp;

	output.valid = true;
	output.complete = true;

	auto time = artdaq::TimeUtils::get_realtime_clock();
	output.atime_ns = time.tv_nsec;
	output.atime_s = time.tv_sec;

	return output;
}
#endif

#endif /* artdaq_core_Data_detail_RawFragmentHeaderV0_hh */
