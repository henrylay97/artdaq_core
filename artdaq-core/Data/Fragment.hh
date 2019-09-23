#ifndef artdaq_core_Data_Fragment_hh
#define artdaq_core_Data_Fragment_hh

#include <algorithm>
//#include <cassert>
#include <stdint.h>
#include <string.h>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include <iostream>
#include "artdaq-core/Core/QuickVec.hh"
#include "artdaq-core/Data/detail/RawFragmentHeader.hh"
#include "artdaq-core/Data/detail/RawFragmentHeaderV0.hh"
#include "artdaq-core/Data/detail/RawFragmentHeaderV1.hh"
#include "artdaq-core/Data/dictionarycontrol.hh"
#if HIDE_FROM_ROOT
#include "trace.h"		// TRACE
#endif

/**
 * \brief The artdaq namespace.
 */
namespace artdaq {
# define DATAVEC_T QuickVec<RawDataType>
	//#define DATAVEC_T std::vector<RawDataType>

	/**
	 * \brief The RawDataType (currently a 64-bit integer) is the basic unit of data representation within _artdaq_
	 *
	 * The RawDataType (currently a 64-bit integer) is the basic unit of data representation within _artdaq_
	 * Copied from RawFragmentHeader into Fragment
	 */
	typedef detail::RawFragmentHeader::RawDataType RawDataType;

	class Fragment;
	/**
	* \brief A std::vector of Fragment objects
	*/
	typedef std::vector<Fragment> Fragments;

	/**
	* \brief A std::unique_ptr to a Fragment object
	*
	* To reduce move or copy operations, most artdaq processing is done
	* using FragmentPtr objects.
	*/
	typedef std::unique_ptr<Fragment> FragmentPtr;

	/**
	* \brief A std::list of FragmentPtrs
	*/
	typedef std::list<FragmentPtr> FragmentPtrs;

	/**
	* \brief A std::pair of a FragmentPtr and the destination rank that
	* the fragment should be sent to
	*/
	typedef std::pair<FragmentPtr, int> PostmarkedFragmentPtr;

	/**
	* \brief A std::list of PostmarkedFragmentPtr objects
	*/
	typedef std::list<PostmarkedFragmentPtr> PostmarkedFragmentPtrs;

	/**
	 * \brief Comparator for Fragment objects, based on their sequence_id
	 * \param i First Fragment to compare
	 * \param j Second Fragment to comapre
	 * \return i.sequenceID() < j.sequenceID()
	 */
	bool fragmentSequenceIDCompare(Fragment i, Fragment j);

	/**
	 * \brief Prints the given Fragment to the stream
	 * \param os Stream to print Fragment to
	 * \param f Fragment to print
	 * \return Reference to the stream
	 */
	std::ostream& operator<<(std::ostream& os, Fragment const& f);
}  // namespace artdaq

/**
 * \brief A Fragment contains the data from one piece of the DAQ system for one event
 * The artdaq::Fragment is the main data storage class in artdaq. Each Fragment represents
 * the data from one piece of the readout, for one artdaq event. BoardReaders create
 * Fragments and send them to the EventBuilders, where they are assembled into
 * artdaq::RawEvent objects.
 */
class artdaq::Fragment
{
public:
	/**
	 * \brief Create a Fragment with all header values zeroed.
	 */
	Fragment();

	/**
	 * \brief For byte representation
	 *
	* JCF, 3/25/14
	* Add interface functions which allow users to work with the
	* underlying data (a vector of RawDataTypes) in byte representation
	 */
	typedef uint8_t byte_t;

	// Hide most things from ROOT.
#if HIDE_FROM_ROOT

	// http://stackoverflow.com/questions/33939687
	// This should generate an exception if artdaq::Fragment is not move-constructible
	/**
	 * \brief Default copy constructor
	 * \todo Decide if Copy constructor should be declared =delete
	 */
	Fragment(const Fragment&) = default;
	/**
	 * \brief Move Constructor
	 *
	 * Separate declaration and definition of Move Constructor:
	 * http://stackoverflow.com/questions/33939687
	 * This should generate an exception if artdaq::Fragment is not move-constructible
	 */
	Fragment(Fragment&&) noexcept;
	/**
	 * \brief Default copy-assignment operator
	 * \return Reference to new Fragment
	 * \todo Decide if copy-assignment operator should be declared =delete
	 */
	Fragment& operator=(const Fragment&) = default;
	/**
	 * \brief Move-assignment operator
	 * \return Reference to Fragment
	 *
	 * Separate declaration and definition of Move Constructor:
	 * http://stackoverflow.com/questions/33939687
	 * This should generate an exception if artdaq::Fragment is not move-constructible
	 */
	Fragment& operator=(Fragment&&) noexcept;

	typedef detail::RawFragmentHeader::version_t version_t; ///< typedef for version_t from RawFragmentHeader
	typedef detail::RawFragmentHeader::type_t type_t; ///< typedef for type_t from RawFragmentHeader
	typedef detail::RawFragmentHeader::sequence_id_t sequence_id_t; ///< typedef for sequence_id_t from RawFragmentHeader
	typedef detail::RawFragmentHeader::fragment_id_t fragment_id_t; ///< typedef for fragment_id_t from RawFragmentHeader
	typedef detail::RawFragmentHeader::timestamp_t timestamp_t; ///< typedef for timestamp_t from RawFragmentHeader

	static constexpr version_t InvalidVersion = detail::RawFragmentHeader::InvalidVersion; ///< Copy InvalidVersion from RawFragmentHeader
	static constexpr sequence_id_t InvalidSequenceID = detail::RawFragmentHeader::InvalidSequenceID; ///< Copy InvalidSequenceID from RawFragmentHeader
	static constexpr fragment_id_t InvalidFragmentID = detail::RawFragmentHeader::InvalidFragmentID; ///< Copy InvalidFragmentID from RawFragmentHeader
	static constexpr timestamp_t InvalidTimestamp = detail::RawFragmentHeader::InvalidTimestamp; ///< Copy InvalidTimestamp from RawFragmentHeader

	static constexpr type_t InvalidFragmentType = detail::RawFragmentHeader::InvalidFragmentType; ///< Copy InvalidFragmentType from RawFragmentHeader
	static constexpr type_t EndOfDataFragmentType = detail::RawFragmentHeader::EndOfDataFragmentType; ///< Copy EndOfDataFragmentType from RawFragmentHeader
	static constexpr type_t DataFragmentType = detail::RawFragmentHeader::DataFragmentType; ///< Copy DataFragmentType from RawFragmentHeader
	static constexpr type_t InitFragmentType = detail::RawFragmentHeader::InitFragmentType; ///< Copy InitFragmentType from RawFragmentHeader
	static constexpr type_t EndOfRunFragmentType = detail::RawFragmentHeader::EndOfRunFragmentType; ///< Copy EndOfRunFragmentType from RawFragmentHeader
	static constexpr type_t EndOfSubrunFragmentType = detail::RawFragmentHeader::EndOfSubrunFragmentType; ///< Copy EndOfSubrunFragmentType from RawFragmentHeader
	static constexpr type_t ShutdownFragmentType = detail::RawFragmentHeader::ShutdownFragmentType; ///< Copy ShutdownFragmentType from RawFragmentHeader
	static constexpr type_t FirstUserFragmentType = detail::RawFragmentHeader::FIRST_USER_TYPE; ///< Copy FIRST_USER_TYPE from RawFragmentHeader
	static constexpr type_t EmptyFragmentType = detail::RawFragmentHeader::EmptyFragmentType; ///< Copy EmptyFragmentType from RawFragmentHeader
	static constexpr type_t ContainerFragmentType = detail::RawFragmentHeader::ContainerFragmentType; ///< Copy ContainerFragmentType from RawFragmentHeader
	static constexpr type_t ErrorFragmentType = detail::RawFragmentHeader::ErrorFragmentType; ///< Copy ErrorFragmentType from RawFragmentHeader

	static constexpr int InvalidDestinationRank = -1;

	/**
	 * \brief Returns whether the given type is in the range of user types
	 * \param fragmentType The type to test
	 * \return Whether the given type is in the range of user types
	 */
	static constexpr bool isUserFragmentType(type_t fragmentType);

	/**
	 * \brief Returns whether the given type is in the range of system types
	 * \param fragmentType  The type to test
	 * \return Whether the given type is in the range of system types
	 */
	static constexpr bool isSystemFragmentType(type_t fragmentType);

	/**
	 * \brief Returns a map of the most commonly-used system types
	 * \return A std::map of the most commonly-used system types
	 */
	static std::map<type_t, std::string> MakeSystemTypeMap()
	{
		return detail::RawFragmentHeader::MakeSystemTypeMap();
	}

	typedef DATAVEC_T::reference reference; ///< Alias reference type from QuickVec<RawDataType>
	typedef DATAVEC_T::iterator iterator; ///< Alias iterator type from QuickVec<RawDataType>
	typedef DATAVEC_T::const_iterator const_iterator; ///< Alias const_iterator type from QuickVec<RawDataType>
	typedef DATAVEC_T::value_type value_type; ///< Alias value_type type from QuickVec<RawDataType>
	typedef DATAVEC_T::difference_type difference_type; ///< Alias difference_type type from QuickVec<RawDataType>
	typedef DATAVEC_T::size_type size_type; ///< Alias size_type type from QuickVec<RawDataType>

	/**
	 * \brief Create a Fragment ready to hold n words (RawDataTypes) of payload, and with
	 * all values zeroed.
	 * \param n The initial size of the Fragment, in RawDataType words
	 */
	explicit Fragment(std::size_t n);

	/**
	 * \brief Create a Fragment using a static factory function rather than a constructor
	 * to allow for the function name "FragmentBytes"
	 * \param nbytes The initial size of the Fragment, in bytes
	 * \return FragmentPtr to created Fragment
	 */
	static FragmentPtr FragmentBytes(std::size_t nbytes)
	{
		RawDataType nwords = ceil(nbytes / static_cast<double>(sizeof(RawDataType)));
		return FragmentPtr(new Fragment(nwords));
	}

	/**
	 * \brief Create a Fragment with the given header values
	 * \tparam T Metadata type
	 * \param payload_size Size of the payload in RawDataType words (Fragment size is header + metadata + payload)
	 * \param sequence_id Sequence ID of Fragment
	 * \param fragment_id Fragment ID of Fragment
	 * \param type Type of Fragment
	 * \param metadata Metadata object
	 * \param timestamp Timestamp of Fragment
	 */
	template <class T>
	Fragment(std::size_t payload_size, sequence_id_t sequence_id,
			 fragment_id_t fragment_id, type_t type, const T& metadata,
			 timestamp_t timestamp = Fragment::InvalidTimestamp);

	/**
	 * \brief Create a Fragment with the given header values. Uses static factory function instead of constructor
	 * to allow for the function name "FragmentBytes"
	 * \tparam T Metadata type
	 * \param payload_size_in_bytes Size of the payload in bytes (Fragment size is header + metadata + payload). Bytes
	 * will be rounded to the next factor of RawDataType / sizeof(char)
	 * \param sequence_id Sequence ID of Fragment
	 * \param fragment_id Fragment ID of Fragment
	 * \param type Type of Fragment
	 * \param metadata Metadata object
	 * \param timestamp Timestamp of Fragment
	 * \return FragmentPtr to created Fragment
	 */
	template <class T>
	static FragmentPtr FragmentBytes(std::size_t payload_size_in_bytes,
									 sequence_id_t sequence_id,
									 fragment_id_t fragment_id,
									 type_t type, const T& metadata,
									 timestamp_t timestamp = Fragment::InvalidTimestamp)
	{
		RawDataType nwords = ceil(payload_size_in_bytes /
								  static_cast<double>(sizeof(RawDataType)));
		return FragmentPtr(new Fragment(nwords, sequence_id, fragment_id, type, metadata, timestamp));
	}

	/**
	 * \brief Create a fragment with the given event id and fragment id, and with no data payload.
	 * \param sequenceID Sequence ID of Fragment
	 * \param fragID Fragment ID of Fragment
	 * \param type Type of Fragment
	 * \param timestamp Timestamp of Fragment
	 */
	Fragment(sequence_id_t sequenceID,
			 fragment_id_t fragID,
			 type_t type = Fragment::DataFragmentType,
			 timestamp_t timestamp = Fragment::InvalidTimestamp);

	/**
	 * \brief Print out summary information for this Fragment to the given stream.
	 * \param os Stream to print to
	 */
	void print(std::ostream& os) const;

	/**
	 * \brief Gets the size of the Fragment, from the Fragment header
	 * \return Number of words in the Fragment. Includes header, metadata, and payload
	 */
	std::size_t size() const;

	/**
	 * \brief Version of the Fragment, from the Fragment header
	 * \return Version of the Fragment
	 */
	version_t version() const;

	/**
	 * \brief Type of the Fragment, from the Fragment header
	 * \return Type of the Fragment
	 */
	type_t type() const;

	/**
	 * \brief Print the type of the Fragment
	 * \return String representation of the Fragment type. For system types, the name will be included in parentheses
	 */
	std::string typeString() const;

	/**
	* \brief Sequence ID of the Fragment, from the Fragment header
	* \return Sequence ID of the Fragment
	*/
	sequence_id_t sequenceID() const;

	/**
	* \brief Fragment ID of the Fragment, from the Fragment header
	* \return Fragment ID of the Fragment
	*/
	fragment_id_t fragmentID() const;

	/**
	* \brief Timestamp of the Fragment, from the Fragment header
	* \return Timestamp of the Fragment
	*/
	timestamp_t timestamp() const;

	/**
	 * \brief Sets the type of the Fragment, checking that it is a valid user type
	 * \param utype The User type to set
	 */
	void setUserType(type_t utype);

	/**
	 * \brief Sets the type of the Fragment, checking that it is a valid system type
	 * \param stype The System type to set
	 */
	void setSystemType(type_t stype);

	/**
	 * \brief Sets the Sequence ID of the Fragment
	 * \param sequence_id The sequence ID to set
	 */
	void setSequenceID(sequence_id_t sequence_id);

	/**
	 * \brief Sets the Fragment ID of the Fragment
	 * \param fragment_id The Fragment ID to set
	 */
	void setFragmentID(fragment_id_t fragment_id);

	/**
	 * \brief Sets the Timestamp of the Fragment
	 * \param timestamp The Timestamp to set
	 */
	void setTimestamp(timestamp_t timestamp);

	/**
	 * \brief Update the access time of the Fragment
	 */
	void touch();

	/**
	 * \brief Get the last access time of the Fragment
	 * \return struct timespec with last access time of the Fragment
	 */
	 struct timespec atime();

	 /**
	  * \brief Get the difference between the current time and the last access time of the Fragment.
	  * \param touch Whether to also perform a touch operation
	  * \return struct timespec representing the difference between current time and the last access time
	  */
	 struct timespec getLatency(bool touch);

	/**
	 * \brief Size of vals_ vector ( header + (optional) metadata + payload) in bytes.
	 * \return The size of the Fragment in bytes, including header, metadata, and payload
	 */
	std::size_t sizeBytes() const { return sizeof(RawDataType) * size(); }

	/**
	 * \brief Return the number of RawDataType words in the data payload. This does not
	 * include the number of words in the header or the metadata.
	 * \return Number of RawDataType words in the payload section of the Fragment
	 */
	std::size_t dataSize() const;

	/**
	 * \brief Return the number of bytes in the data payload. This does not
	 * include the number of bytes in the header or the metadata.
	 * \return
	 */
	std::size_t dataSizeBytes() const
	{
		return sizeof(RawDataType) * dataSize();
	}

	/**
	 * \brief Test whether this Fragment has metadata
	 * \return If a metadata object has been set
	 */
	bool hasMetadata() const;

	/**
	 * \brief Return a pointer to the metadata. This throws an exception
	 * if the Fragment contains no metadata.
	 * \tparam T Type of the metadata
	 * \return Pointer to the metadata
	 * \exception cet::exception if no metadata is present
	 */
	template <class T>
	T* metadata();

	/**
	 * \brief Return a const pointer to the metadata. This throws an exception
	 * if the Fragment contains no metadata.
	 * \tparam T Type of the metadata
	 * \return const Pointer to the metadata
	 * \exception cet::exception if no metadata is present
	 */
	template <class T>
	T const* metadata() const;

	/**
	 * \brief Set the metadata in the Fragment to the contents of
	 * the specified structure.  This throws an exception if
	 * the Fragment already contains metadata.
	 * \tparam T Type of the metadata
	 * \param md Metadata to store in Fragment
	 * \exception cet::exception if metadata already present in Fragment
	 */
	template <class T>
	void setMetadata(const T& md);

	/**
	 * \brief Updates existing metadata with a new metadata object
	 * \tparam T Type of the metadata
	 * \param md Metadata to set
	 * \exception cet::exception if no metadata stored in Fragment
	 * \exception cet::exception if new metadata has different size than existing metadata
	 */
	template <class T>
	void updateMetadata(const T& md);

	/**
	 * \brief Resize the data payload to hold sz RawDataType words.
	 * \param sz The new size of the payload portion of the Fragment, in RawDataType words
	 */
	void resize(std::size_t sz);

	/**
	 * \brief Resize the data payload to hold sz RawDataType words. Initialize new elements (if any) with val
	 * \param sz The new size of the payload portion of the Fragment, in RawDataType words
	 * \param val Value with which to initialize any new elements
	 */
	void resize(std::size_t sz, RawDataType val);

	/**
	 * \brief Resize the data payload to hold szbytes bytes (padded by the
	 * 8-byte RawDataTypes, so, e.g., requesting 14 bytes will actually
	 * get you 16)
	 * \param szbytes The new size of the payload portion of the Fragment, in bytes
	 */
	void resizeBytes(std::size_t szbytes);

	/**
	 * \brief Resize the data payload to hold szbytes bytes (padded by the
	 * 8-byte RawDataTypes, so, e.g., requesting 14 bytes will actually
	 * get you 16) and request additional capacity in the underlying storage
	 * (to help avoid extra reallocations)
	 * \param szbytes The new size of the payload portion of the Fragment, in bytes
	 * \param growthFactor The requested growth factor in the capacity of storage
	 */
	void resizeBytesWithCushion(std::size_t szbytes, double growthFactor=1.3);

	/**
	* \brief Resize the data payload to hold sz bytes (padded by the
	 * 8-byte RawDataTypes, so, e.g., requesting 14 bytes will actually
	 * get you 16). Initialize new elements (if any) with val.
	* \param szbytes The new size of the payload portion of the Fragment, in bytes
	* \param val Value with which to initialize any new elements
	*/
	void resizeBytes(std::size_t szbytes, byte_t val);

	/**
	 * \brief  Resize the fragment to hold the number of words indicated by the header.
	 */
	void autoResize();

	/**
	 * \brief Return an iterator to the beginning of the data payload (after header and metadata)
	 * \return iterator to the beginning of the data payload
	 */
	iterator dataBegin();

	/**
	 * \brief Return an iterator to the end of the data payload
	 * \return iterator to the end of the data payload
	 */
	iterator dataEnd();

	/**
	 * \brief Wrapper around reinterpret_cast
	 * \tparam T Type of output pointer
	 * \param in input pointer
	 * \return Pointer cast to type T
	 * \exception cet::exception if new pointer does not point to same address as old pointer
	 *
	 *  JCF, 1/21/15
	 * There's actually not an ironclad guarantee in the C++ standard
	 * that the pointer reinterpret_cast<> returns has the same address
	 * as the pointer that was casted. It IS tested in the artdaq-core
	 * test suite, but since any uncaught, unexpected behavior from
	 * reinterpret_cast could be disastrous, I've wrapped it in this
	 * function and added a check just to be completely safe.
	 *
	 * Please note that for this const-version, you'll need the const-
	 * qualifier to the pointer you pass as a parameter (i.e.,
	 * reinterpret_cast_checked<const PtrType*>, not reinterpret_cast_checked<PtrType*>)
	 */
	template <typename T>
	T reinterpret_cast_checked(const RawDataType* in) const
	{
		T newpointer = reinterpret_cast<T>(in);

		if (static_cast<const void*>(newpointer) != static_cast<const void*>(in))
		{
			throw cet::exception("Error in Fragment.hh: reinterpret_cast failed to return expected address-- please contact John Freeman at jcfree@fnal.gov");
		}

		return newpointer;
	}

	/**
	* \brief Wrapper around reinterpret_cast
	* \tparam T Type of output pointer
	* \param in input pointer
	* \return Pointer cast to type T
	* \exception cet::exception if new pointer does not point to same address as old pointer
	*
	*  JCF, 1/21/15
	* There's actually not an ironclad guarantee in the C++ standard
	* that the pointer reinterpret_cast<> returns has the same address
	* as the pointer that was casted. It IS tested in the artdaq-core
	* test suite, but since any uncaught, unexpected behavior from
	* reinterpret_cast could be disastrous, I've wrapped it in this
	* function and added a check just to be completely safe.
	*/
	template <typename T>
	T reinterpret_cast_checked(RawDataType* in)
	{
		T newpointer = reinterpret_cast<T>(in);

		if (static_cast<void*>(newpointer) != static_cast<void*>(in))
		{
			throw cet::exception("Error in Fragment.hh: reinterpret_cast failed to return expected address-- please contact John Freeman at jcfree@fnal.gov");
		}

		return newpointer;
	}

	/**
	 * \brief Return Fragment::byte_t* pointing at the beginning of the payload
	 * \return byte_t pointer to beginning of data payload
	 *
	 * JCF, 3/25/14 -- one nice thing about returning a pointer rather
	 * than an iterator is that we don't need to take the address of the
	 * dereferenced iterator (e.g., via &*dataBegin() ) to get ahold of the memory
	 */
	byte_t* dataBeginBytes() { return reinterpret_cast_checked<byte_t*>(&* dataBegin()); }

	/**
	* \brief Return Fragment::byte_t* pointing at the end of the payload
	* \return byte_t pointer to end of data payload
	*
	* JCF, 3/25/14 -- one nice thing about returning a pointer rather
	* than an iterator is that we don't need to take the address of the
	* dereferenced iterator (e.g., via &*dataEnd() ) to get ahold of the memory
	*/
	byte_t* dataEndBytes() { return reinterpret_cast_checked<byte_t*>(&* dataEnd()); }

	/**
	 * \brief Return an iterator to the beginning of the header (should be used
	 * for serialization only: use setters for preference).
	 * \return an iterator to the beginning of the header
	 */
	iterator headerBegin();

	/**
	 * \brief Return a Fragment::byte_t pointer pointing to the beginning of the header
	 * \return byte_t pointer to the beginning of the header
	 */
	byte_t* headerBeginBytes() { return reinterpret_cast_checked<byte_t*>(&* headerBegin()); }

	/**
	 * \brief Returns a const_iterator to the beginning of the data payload
	 * \return A const_iterator to the beginning of the data payload
	 */
	const_iterator dataBegin() const;

	/**
	* \brief Returns a const_iterator to the end of the data payload
	* \return A const_iterator to the end of the data payload
	*/
	const_iterator dataEnd() const;

	/**
	* \brief Return const Fragment::byte_t* pointing at the beginning of the payload
	* \return const byte_t pointer to beginning of data payload
	*
	* JCF, 3/25/14 -- one nice thing about returning a pointer rather
	* than an iterator is that we don't need to take the address of the
	* dereferenced iterator (e.g., via &*dataEnd() ) to get ahold of the memory
	*/
	const byte_t* dataBeginBytes() const
	{
		return reinterpret_cast_checked<const byte_t*>(&* dataBegin());
	}

	/**
	* \brief Return const Fragment::byte_t* pointing at the end of the payload
	* \return const byte_t pointer to end of data payload
	*
	* JCF, 3/25/14 -- one nice thing about returning a pointer rather
	* than an iterator is that we don't need to take the address of the
	* dereferenced iterator (e.g., via &*dataEnd() ) to get ahold of the memory
	*/
	const byte_t* dataEndBytes() const
	{
		return reinterpret_cast_checked<const byte_t*>(&* dataEnd());
	}

	/**
	* \brief Return an const_iterator to the beginning of the header (should be used
	* for serialization only: use setters for preference).
	* \return an const_iterator to the beginning of the header
	*/
	const_iterator headerBegin() const; // See note for non-const, above.

	/**
	* \brief Return a const Fragment::byte_t pointer pointing to the beginning of the header
	* \return const byte_t pointer to the beginning of the header
	*/
	const byte_t* headerBeginBytes() const
	{
		return reinterpret_cast_checked<const byte_t*>(&* headerBegin());
	}

	/**
	 * \brief Removes all elements from the payload section of the Fragment
	 */
	void clear();

	/**
	 * \brief Determines if the Fragment contains no data
	 * \return Whether the Fragment's payload is empty
	 */
	bool empty();

	/**
	 * \brief Reserves enough memory to hold cap RawDataType words in the Fragment payload
	 * \param cap The new capacity of the Fragment payload, in RawDataType words.
	 */
	void reserve(std::size_t cap);

	/**
	 * \brief Swaps two Fragment objects
	 * \param other Fragment to swap with
	 */
	void swap(Fragment& other) noexcept;

	/**
	 * \brief Swaps two Fragment data vectors
	 * \param other The data vector to swap with
	 *
	 * Since all Fragment header information is stored in the data vector, this is equivalent to swapping two Fragment objects
	 */
	void swap(DATAVEC_T& other) noexcept { vals_.swap(other); };

	/**
	 * \brief Returns a RawDataType pointer to the beginning of the payload
	 * \return A RawDataType pointer to the beginning of the payload
	 */
	RawDataType* dataAddress();

	/**
	 * \brief Get the address of the metadata. For internal use only, use metadata() instead.
	 * \return Pointer to the metadata's location within the vals_ vector
	 * \exception cet::exception if no metadata in Fragment
	 * \todo Change function access specifier to restrict access
	 */
	RawDataType* metadataAddress();
	/**
	 * \brief Gets the address of the header.
	 * \return Pointer to the header's location within the vals_ vector
	 */
	RawDataType* headerAddress();

	/**
	 * \brief Creates an EndOfData Fragment
	 * \param nFragsToExpect The number of Fragments the receiver should have at the end of data-taking
	 * \return Pointer to created EndOfData Fragment
	 */
	static FragmentPtr eodFrag(size_t nFragsToExpect);

	/**
	 * \brief Creates a Fragment, copying data from given location.
	 * 12-Apr-2013, KAB - this method is deprecated, please do not use (internal use only)
	 * \tparam InputIterator Type of input iterator
	 * \param sequenceID Sequence ID of new Fragment
	 * \param fragID Fragment ID of new Fragment
	 * \param i Beginning of input range
	 * \param e End of input range
	 * \return FragmentPtr to created Fragment
	 * \todo Change function access specifier to restrict access
	 */
	template<class InputIterator>
	static FragmentPtr dataFrag(sequence_id_t sequenceID,
															  fragment_id_t fragID,
															  InputIterator i,
															  InputIterator e)
	{
		FragmentPtr result(new Fragment(sequenceID, fragID));
		result->vals_.reserve(std::distance(i, e) + detail::RawFragmentHeader::num_words());
		std::copy(i, e, std::back_inserter(result->vals_));
		result->updateFragmentHeaderWC_();
		return result;
	}

	/**
	 * \brief Crates a Fragment, copying data from given location.
	 * \param sequenceID Sequence ID of new Fragment
	 * \param fragID Fragment ID of new Fragment
	 * \param dataPtr Pointer to data to store in Fragment
	 * \param dataSize Size of data to store in Fragment
	 * \param timestamp Timestamp of created Fragment
	 * \return FragmentPtr to created Fragment
	 */
	static FragmentPtr dataFrag(sequence_id_t sequenceID,
								fragment_id_t fragID,
								RawDataType const* dataPtr,
								size_t dataSize,
								timestamp_t timestamp = Fragment::InvalidTimestamp);
#endif

private:
	template <typename T>
	static std::size_t validatedMetadataSize_();

	void updateFragmentHeaderWC_();

	DATAVEC_T vals_;

#if HIDE_FROM_ROOT
	detail::RawFragmentHeader* fragmentHeader();

	detail::RawFragmentHeader const* fragmentHeader() const;
#endif
};

#if HIDE_FROM_ROOT

// http://stackoverflow.com/questions/33939687
// This should generate an exception if artdaq::Fragment is not move-constructible
inline artdaq::Fragment::Fragment(artdaq::Fragment&&) noexcept = default;
inline artdaq::Fragment& artdaq::Fragment::operator=(artdaq::Fragment&&) noexcept = default;

inline bool constexpr artdaq::Fragment::
isUserFragmentType(type_t fragmentType)
{
	return fragmentType >= detail::RawFragmentHeader::FIRST_USER_TYPE &&
		fragmentType <= detail::RawFragmentHeader::LAST_USER_TYPE;
}

inline bool constexpr artdaq::Fragment::
isSystemFragmentType(type_t fragmentType)
{
	return fragmentType >= detail::RawFragmentHeader::FIRST_SYSTEM_TYPE;
}

template <typename T>
std::size_t
artdaq::Fragment::
validatedMetadataSize_()
{
	// Make sure a size_t is big enough to hold the maximum metadata
	// size. This *should* always be true, but it is a compile-time check
	// and therefore cheap.
	static_assert(sizeof(size_t) >=
				  sizeof(decltype(std::numeric_limits<detail::RawFragmentHeader::metadata_word_count_t>::max())),
				  "metadata_word_count_t is too big!");

	static size_t constexpr max_md_wc =
		std::numeric_limits<detail::RawFragmentHeader::metadata_word_count_t>::max();
	size_t requested_md_wc =
		std::ceil(sizeof(T) / static_cast<double>(sizeof(artdaq::RawDataType)));
	if (requested_md_wc > max_md_wc)
	{
		throw cet::exception("InvalidRequest")
			<< "The requested metadata structure is too large: "
			<< "requested word count = " << requested_md_wc
			<< ", maximum word count = " << max_md_wc;
	}
	return requested_md_wc;
}

template <class T>
artdaq::Fragment::
Fragment(std::size_t payload_size, sequence_id_t sequence_id,
		 fragment_id_t fragment_id,
             type_t type, const T& metadata, timestamp_t timestamp)
    : vals_((artdaq::detail::RawFragmentHeader::num_words() +  // Header
		   validatedMetadataSize_<T>() + // Metadata
		   payload_size) // User data
		  )
{
	TRACEN("Fragment", 50, "Fragment ctor num_word()=%zu MetadataSize_=%zu payload_size=%zu", artdaq::detail::RawFragmentHeader::num_words(), validatedMetadataSize_<T>(), payload_size);
	// vals ctor w/o init val is used; make sure header is ALL initialized.
	for (iterator ii = vals_.begin();
	     ii != (vals_.begin() + detail::RawFragmentHeader::num_words()); ++ii)
	{
		*ii = -1;
	}
	fragmentHeader()->version = detail::RawFragmentHeader::CurrentVersion;
	updateFragmentHeaderWC_();
	fragmentHeader()->sequence_id = sequence_id;
	fragmentHeader()->fragment_id = fragment_id;
	fragmentHeader()->timestamp = timestamp;
	fragmentHeader()->type = type;

	fragmentHeader()->touch();

	fragmentHeader()->metadata_word_count =
		vals_.size() -
		(fragmentHeader()->num_words() + payload_size);

	memcpy(metadataAddress(), &metadata, sizeof(T));
}

inline std::size_t
artdaq::Fragment::size() const
{
	return fragmentHeader()->word_count;
}

inline artdaq::Fragment::version_t
artdaq::Fragment::version() const
{
	return fragmentHeader()->version;
}

inline artdaq::Fragment::type_t
artdaq::Fragment::type() const
{
	return static_cast<type_t>(fragmentHeader()->type);
}

inline std::string
artdaq::Fragment::typeString() const
{
	return std::to_string(type()) + (isSystemFragmentType(type()) ? " (" + detail::RawFragmentHeader::SystemTypeToString(type()) + ")" : "");
}

inline artdaq::Fragment::sequence_id_t
artdaq::Fragment::sequenceID() const
{
	return fragmentHeader()->sequence_id;
}

inline artdaq::Fragment::fragment_id_t
artdaq::Fragment::fragmentID() const
{
	return fragmentHeader()->fragment_id;
}

inline artdaq::Fragment::timestamp_t
artdaq::Fragment::timestamp() const
{
	return fragmentHeader()->timestamp;
}

inline void
artdaq::Fragment::setUserType(type_t type)
{
	fragmentHeader()->setUserType(static_cast<uint8_t>(type));
}

inline void
artdaq::Fragment::setSystemType(type_t type)
{
	fragmentHeader()->setSystemType(static_cast<uint8_t>(type));
}

inline void
artdaq::Fragment::setSequenceID(sequence_id_t sequence_id)
{
	assert(sequence_id <= detail::RawFragmentHeader::InvalidSequenceID);
	fragmentHeader()->sequence_id = sequence_id;
}

inline void
artdaq::Fragment::setFragmentID(fragment_id_t fragment_id)
{
	fragmentHeader()->fragment_id = fragment_id;
}

inline void
artdaq::Fragment::setTimestamp(timestamp_t timestamp)
{
	fragmentHeader()->timestamp = timestamp;
}

inline void artdaq::Fragment::touch()
{
	fragmentHeader()->touch();
}

inline struct timespec artdaq::Fragment::atime()
{
	return fragmentHeader()->atime();
}

inline struct timespec artdaq::Fragment::getLatency(bool touch)
{
	return fragmentHeader()->getLatency(touch);
}

inline void
artdaq::Fragment::updateFragmentHeaderWC_()
{
	// Make sure vals_.size() fits inside 32 bits. Left-shift here should
	// match bitfield size of word_count in RawFragmentHeader.
	assert(vals_.size() < (1ULL << 32));
	TRACEN( "Fragment", 50, "Fragment::updateFragmentHeaderWC_ adjusting fragmentHeader()->word_count from %u to %zu", (unsigned)(fragmentHeader()->word_count), vals_.size() );
	fragmentHeader()->word_count = vals_.size();
}

inline std::size_t
artdaq::Fragment::dataSize() const
{
	return vals_.size() - fragmentHeader()->num_words() -
		fragmentHeader()->metadata_word_count;
}

inline bool
artdaq::Fragment::hasMetadata() const
{
	return fragmentHeader()->metadata_word_count != 0;
}

template <class T>
T* artdaq::Fragment::metadata()
{
	if (fragmentHeader()->metadata_word_count == 0)
	{
		throw cet::exception("InvalidRequest")
			<< "No metadata has been stored in this Fragment.";
	}

	return reinterpret_cast_checked<T*>(&vals_[fragmentHeader()->num_words()]);
}

template <class T>
T const*
artdaq::Fragment::metadata() const
{
	if (fragmentHeader()->metadata_word_count == 0)
	{
		throw cet::exception("InvalidRequest")
			<< "No metadata has been stored in this Fragment.";
	}
	return reinterpret_cast_checked<T const*>(&vals_[fragmentHeader()->num_words()]);
}

template <class T>
void artdaq::Fragment::setMetadata(const T& metadata)
{
	if (fragmentHeader()->metadata_word_count != 0)
	{
		throw cet::exception("InvalidRequest")
			<< "Metadata has already been stored in this Fragment.";
	}
	auto const mdSize = validatedMetadataSize_<T>();
	vals_.insert(dataBegin(), mdSize, 0);
	updateFragmentHeaderWC_();
	fragmentHeader()->metadata_word_count = mdSize;

	memcpy(metadataAddress(), &metadata, sizeof(T));
}

template <class T>
void artdaq::Fragment::updateMetadata(const T& metadata)
{
	if (fragmentHeader()->metadata_word_count == 0)
	{
		throw cet::exception("InvalidRequest")
			<< "No metadata in fragment; please use Fragment::setMetadata instead of Fragment::updateMetadata";
	}

	auto const mdSize = validatedMetadataSize_<T>();

	if (fragmentHeader()->metadata_word_count != mdSize)
	{
		throw cet::exception("InvalidRequest")
			<< "Mismatch between type of metadata struct passed to updateMetadata and existing metadata struct";
	}

	memcpy(metadataAddress(), &metadata, sizeof(T));
}

inline void
artdaq::Fragment::resize(std::size_t sz)
{
	vals_.resize(sz + fragmentHeader()->metadata_word_count +
				 fragmentHeader()->num_words());
	updateFragmentHeaderWC_();
}

inline void
artdaq::Fragment::resize(std::size_t sz, RawDataType v)
{
	vals_.resize(sz + fragmentHeader()->metadata_word_count +
	                 fragmentHeader()->num_words(),
	             v);
	updateFragmentHeaderWC_();
}

inline void
artdaq::Fragment::resizeBytes(std::size_t szbytes)
{
	RawDataType nwords = ceil(szbytes / static_cast<double>(sizeof(RawDataType)));
	resize(nwords);
}

inline void
artdaq::Fragment::resizeBytesWithCushion(std::size_t szbytes, double growthFactor)
{
	RawDataType nwords = ceil(szbytes / static_cast<double>(sizeof(RawDataType)));
	vals_.resizeWithCushion(nwords + fragmentHeader()->metadata_word_count +
	                            fragmentHeader()->num_words(),
	                        growthFactor);
	updateFragmentHeaderWC_();
}

inline void
artdaq::Fragment::resizeBytes(std::size_t szbytes, byte_t v)
{
	RawDataType defaultval;
	byte_t* ptr = reinterpret_cast_checked<byte_t*>(&defaultval);

	for (uint8_t i = 0; i < sizeof(RawDataType); ++i)
	{
		*ptr = v;
		ptr++;
	}

	RawDataType nwords = ceil(szbytes / static_cast<double>(sizeof(RawDataType)));

	resize(nwords, defaultval);
}

inline void
artdaq::Fragment::autoResize()
{
	vals_.resize(fragmentHeader()->word_count);
	updateFragmentHeaderWC_();
}

inline artdaq::Fragment::iterator
artdaq::Fragment::dataBegin()
{
	return vals_.begin() + fragmentHeader()->num_words() +
		fragmentHeader()->metadata_word_count;
}

inline artdaq::Fragment::iterator
artdaq::Fragment::dataEnd()
{
	return vals_.end();
}

inline artdaq::Fragment::iterator
artdaq::Fragment::headerBegin()
{
	return vals_.begin();
}

inline artdaq::Fragment::const_iterator
artdaq::Fragment::dataBegin() const
{
	return vals_.begin() + fragmentHeader()->num_words() +
		fragmentHeader()->metadata_word_count;
}

inline artdaq::Fragment::const_iterator
artdaq::Fragment::dataEnd() const
{
	return vals_.end();
}

inline artdaq::Fragment::const_iterator
artdaq::Fragment::headerBegin() const
{
	return vals_.begin();
}

inline void
artdaq::Fragment::clear()
{
	vals_.erase(dataBegin(), dataEnd());
	updateFragmentHeaderWC_();
}

inline bool
artdaq::Fragment::empty()
{
	return (vals_.size() - fragmentHeader()->num_words() -
			fragmentHeader()->metadata_word_count) == 0;
}

inline void
artdaq::Fragment::reserve(std::size_t cap)
{
	vals_.reserve(cap + fragmentHeader()->num_words() +
				  fragmentHeader()->metadata_word_count);
}

inline void
artdaq::Fragment::swap(Fragment& other) noexcept
{
	vals_.swap(other.vals_);
}

inline artdaq::RawDataType*
artdaq::Fragment::dataAddress()
{
	return &vals_[0] + fragmentHeader()->num_words() +
		fragmentHeader()->metadata_word_count;
}

inline artdaq::RawDataType*
artdaq::Fragment::metadataAddress()
{
	if (fragmentHeader()->metadata_word_count == 0)
	{
		throw cet::exception("InvalidRequest")
			<< "No metadata has been stored in this Fragment.";
	}
	return &vals_[0] + fragmentHeader()->num_words();
}

inline artdaq::RawDataType*
artdaq::Fragment::headerAddress()
{
	return &vals_[0];
}

inline artdaq::detail::RawFragmentHeader*
artdaq::Fragment::fragmentHeader()
{
	auto hdr = reinterpret_cast_checked<detail::RawFragmentHeader *>(&vals_[0]);
	if (hdr->version != detail::RawFragmentHeader::CurrentVersion)
	{
		switch (hdr->version)
		{
		case 0xFFFF:
			//std::cout << "Not upgrading InvalidVersion Fragment" << std::endl;
			break;
		case 0:
		{
				//std::cout << "Upgrading RawFragmentHeaderV0 (non const)" << std::endl;
				TRACEN("Fragment", 4, "Upgrading RawFragmentHeaderV0 (non const)");
			auto old_hdr = reinterpret_cast_checked<detail::RawFragmentHeaderV0 *>(&vals_[0]);
			auto new_hdr = old_hdr->upgrade();

			auto szDiff = hdr->num_words() - old_hdr->num_words();
			if (szDiff > 0) vals_.insert(vals_.begin(), szDiff, 0);
			memcpy(&vals_[0], &new_hdr, hdr->num_words() * sizeof(RawDataType));
				hdr = reinterpret_cast_checked<detail::RawFragmentHeader*>(&vals_[0]);  // Update hdr in case vals_->insert call invalidated pointers
			break;
		}
		case 1:
		{
				//std::cout << "Upgrading RawFragmentHeaderV1 (non const)" << std::endl;
				TRACEN("Fragment", 4, "Upgrading RawFragmentHeaderV1 (non const)");
			auto old_hdr = reinterpret_cast_checked<detail::RawFragmentHeaderV1 *>(&vals_[0]);
			auto new_hdr = old_hdr->upgrade();

			auto szDiff = hdr->num_words() - old_hdr->num_words();
			if (szDiff > 0) vals_.insert(vals_.begin(), szDiff, 0);
			memcpy(&vals_[0], &new_hdr, hdr->num_words() * sizeof(RawDataType));
				hdr = reinterpret_cast_checked<detail::RawFragmentHeader*>(&vals_[0]);  // Update hdr in case vals_->insert call invalidated pointers
		break;
		}
		default:
			throw cet::exception("Fragment") << "A Fragment with an unknown version (" << std::to_string(hdr->version) << ") was received!";
			break;
		}
	}
	return hdr;
}

inline artdaq::detail::RawFragmentHeader const*
artdaq::Fragment::fragmentHeader() const
{
	auto hdr = reinterpret_cast_checked<detail::RawFragmentHeader const*>(&vals_[0]);
	if (hdr->version != detail::RawFragmentHeader::CurrentVersion)
	{
		switch (hdr->version)
		{
		case 0xFFFF:
			//std::cout << "Not upgrading InvalidVersion Fragment" << std::endl;
			break;
		case 0:
		{
				//std::cout << "Upgrading RawFragmentHeaderV0 (const)" << std::endl;
				TRACEN("Fragment", 4, "Upgrading RawFragmentHeaderV0 (const)");
			auto old_hdr = reinterpret_cast_checked<detail::RawFragmentHeaderV0 const*>(&vals_[0]);
			auto new_hdr = old_hdr->upgrade();

			auto szDiff = hdr->num_words() - old_hdr->num_words();
			auto vals_nc = const_cast<DATAVEC_T*>(&vals_);
			if (szDiff > 0) vals_nc->insert(vals_nc->begin(), szDiff, 0);
			memcpy(&(*vals_nc)[0], &new_hdr, hdr->num_words() * sizeof(RawDataType));
				hdr = reinterpret_cast_checked<detail::RawFragmentHeader const*>(&vals_[0]);  // Update hdr in case vals_nc->insert call invalidated pointers
			break;
		}
		case 1:
		{
				//std::cout << "Upgrading RawFragmentHeaderV1 (const)" << std::endl;
				TRACEN("Fragment", 4, "Upgrading RawFragmentHeaderV1 (const)");
			auto old_hdr = reinterpret_cast_checked<detail::RawFragmentHeaderV1 const*>(&vals_[0]);
			auto new_hdr = old_hdr->upgrade();

			auto szDiff = hdr->num_words() - old_hdr->num_words();
			auto vals_nc = const_cast<DATAVEC_T*>(&vals_);
			if (szDiff > 0) vals_nc->insert(vals_nc->begin(), szDiff, 0);
			memcpy(&(*vals_nc)[0], &new_hdr, hdr->num_words() * sizeof(RawDataType));
				hdr = reinterpret_cast_checked<detail::RawFragmentHeader const*>(&vals_[0]);  // Update hdr in case vals_nc->insert call invalidated pointers
		break;
		}
		default:
			throw cet::exception("Fragment") << "A Fragment with an unknown version (" << std::to_string(hdr->version) << ") was received!";
			break;
		}
	}
	return hdr;
}

inline void
swap(artdaq::Fragment& x, artdaq::Fragment& y) noexcept
{
	x.swap(y);
}

inline std::ostream&
artdaq::operator<<(std::ostream& os, artdaq::Fragment const& f)
{
	f.print(os);
	return os;
}
#endif/* HIDE_FROM_ROOT */

#endif /* artdaq_core_Data_Fragment_hh */
