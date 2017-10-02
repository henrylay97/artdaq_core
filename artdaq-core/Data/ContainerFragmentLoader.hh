#ifndef artdaq_core_Data_ContainerFragmentLoader_hh
#define artdaq_core_Data_ContainerFragmentLoader_hh

////////////////////////////////////////////////////////////////////////
// ContainerFragmentLoader
//
// This class gives write access to a ContainerFragment. It should be
// used when multiple fragments are generated by one BoardReader for a
// single event.
//
////////////////////////////////////////////////////////////////////////

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "tracemf.h"

#include <iostream>

namespace artdaq
{
	class ContainerFragmentLoader;
}


/**
 * \brief A Read-Write version of the ContainerFragment, used for filling ContainerFragment objects with other Fragment objects
 */
class artdaq::ContainerFragmentLoader : public artdaq::ContainerFragment
{
public:


	/**
	 * \brief Constructs the ContainerFragmentLoader
	 * \param f A Fragment object containing a Fragment header.
	 * \param expectedFragmentType The type of fragment which will be put into this ContainerFragment
	 * \exception cet::exception if the Fragment input has inconsistent Header information
	 */
	explicit ContainerFragmentLoader(Fragment& f, Fragment::type_t expectedFragmentType);

	// ReSharper disable once CppMemberFunctionMayBeConst
	/**
	 * \brief Get the ContainerFragment metadata (includes information about the location of Fragment objects within the ContainerFragment)
	 * \return The ContainerFragment metadata
	 */
	Metadata* metadata()
	{
		assert(artdaq_Fragment_.hasMetadata());
		return reinterpret_cast<Metadata *>(&*artdaq_Fragment_.metadataAddress());
	}

	/**
	 * \brief Sets the type of Fragment which this ContainerFragment should contain
	 * \param type The Fragment::type_t identifying the type of Fragment objects stored in this ContainerFragment
	 */
	void set_fragment_type(Fragment::type_t type)
	{
		metadata()->fragment_type = type;
	}

	/**
	 * \brief Sets the missing_data flag
	 * \param isDataMissing The value of the missing_data flag
	 *
	 * The ContainerFragment::Metadata::missing_data flag is used for FragmentGenerators to indicate that the fragment is incomplete,
	 * but the generator does not have the correct data to fill it. This happens for Window-mode FragmentGenerators when the window
	 * requested is before the start of the FragmentGenerator's buffers, for example.
	 */
	void set_missing_data(bool isDataMissing)
	{
		metadata()->missing_data = isDataMissing;
	}

	/**
	 * \brief Add a Fragment to the ContainerFragment by reference
	 * \param frag A Fragment object to be added to the ContainerFragment
	 * \exception cet::exception If the Fragment to be added has a different type than expected
	 */
	void addFragment(artdaq::Fragment& frag);

	/**
	 * \brief Add a Fragment to the ContainerFragment by smart pointer
	 * \param frag A FragmentPtr to a Fragment to be added to the ContainerFragment
	 */
	void addFragment(artdaq::FragmentPtr& frag);

	/**
	 * \brief Add a collection of Fragment objects to the ContainerFragment
	 * \param frags An artdaq::FragmentPtrs object containing Fragments to be added to the ContainerFragment
	 */
	void addFragments(artdaq::FragmentPtrs& frags);

private:
	// Note that this non-const reference hides the const reference in the base class
	artdaq::Fragment& artdaq_Fragment_;

	static size_t words_to_frag_words_(size_t nWords);

	void addSpace_(size_t bytes);

	uint8_t* dataBegin_() { return reinterpret_cast<uint8_t*>(&*artdaq_Fragment_.dataBegin()); }
	void* dataEnd_() { return reinterpret_cast<void*>(dataBegin_() + lastFragmentIndex()); }
};

inline artdaq::ContainerFragmentLoader::ContainerFragmentLoader(artdaq::Fragment& f, artdaq::Fragment::type_t expectedFragmentType = Fragment::EmptyFragmentType) :
	ContainerFragment(f)
	, artdaq_Fragment_(f)
{
	artdaq_Fragment_.setSystemType(Fragment::ContainerFragmentType);
	Metadata m;
	m.block_count = 0;
	m.fragment_type = expectedFragmentType;
	m.missing_data = false;
	for (int ii = 0; ii < CONTAINER_FRAGMENT_COUNT_MAX; ++ii)
	{
		m.index[ii] = 0;
	}
	artdaq_Fragment_.setMetadata<Metadata>(m);

	if (artdaq_Fragment_.size() !=
		artdaq::detail::RawFragmentHeader::num_words() +
		words_to_frag_words_(Metadata::size_words))
	{
		TLOG_ERROR("ContainerFragmentLoader") << "ContainerFragmentLoader: Raw artdaq::Fragment object size suggests it does not consist of its own header + the ContainerFragment::Metadata object" << TLOG_ENDL;
		TLOG_ERROR("ContainerFragmentLoader") <<"artdaq_Fragment size: " << artdaq_Fragment_.size() << ", Expected size: " << artdaq::detail::RawFragmentHeader::num_words() +	words_to_frag_words_(Metadata::size_words) << TLOG_ENDL;

		throw cet::exception("InvalidFragment") << "ContainerFragmentLoader: Raw artdaq::Fragment object size suggests it does not consist of its own header + the ContainerFragment::Metadata object";
	}
}

inline size_t artdaq::ContainerFragmentLoader::words_to_frag_words_(size_t nWords)
{
	size_t mod = nWords % words_per_frag_word_();
	return mod ?
		nWords / words_per_frag_word_() + 1 :
		nWords / words_per_frag_word_();
}

inline void artdaq::ContainerFragmentLoader::addSpace_(size_t bytes)
{
	auto currSize = sizeof(artdaq::Fragment::value_type) * artdaq_Fragment_.dataSize(); // Resize takes into account header and metadata size
	artdaq_Fragment_.resizeBytes(bytes + currSize);
	TRACE(4, "ContainerFragmentLoader::addSpace_: dataEnd_ is now at %p", dataEnd_());
}

inline void artdaq::ContainerFragmentLoader::addFragment(artdaq::Fragment& frag)
{
	if (metadata()->block_count >= CONTAINER_FRAGMENT_COUNT_MAX)
	{
		TLOG_ERROR("ContainerFragmentLoader") << "addFragment: Fragment is full, cannot add more fragments!" << TLOG_ENDL;
		throw cet::exception("ContainerFull") << "ContainerFragmentLoader::addFragment: Fragment is full, cannot add more fragments!";
	}

	TRACE(4, "ContainerFragmentLoader::addFragment: Adding Fragment with payload size %llu to Container", (unsigned long long)frag.dataSizeBytes());
	if (metadata()->fragment_type == Fragment::EmptyFragmentType) metadata()->fragment_type = frag.type();
	else if (frag.type() != metadata()->fragment_type)
	{
		TLOG_ERROR("ContainerFragmentLoader") << "addFragment: Trying to add a fragment of different type than what's already been added!" << TLOG_ENDL;
		throw cet::exception("WrongFragmentType") << "ContainerFragmentLoader::addFragment: Trying to add a fragment of different type than what's already been added!";
	}
	TRACE(4, "ContainerFragmentLoader::addFragment: Payload Size is %llu, lastFragmentIndex is %llu, and frag.size is %llu", (unsigned long long)artdaq_Fragment_.dataSizeBytes(), (unsigned long long)lastFragmentIndex(), (unsigned long long)frag.sizeBytes());
	if (artdaq_Fragment_.dataSizeBytes() < lastFragmentIndex() + frag.sizeBytes())
	{
		addSpace_(frag.sizeBytes());
	}
	frag.setSequenceID(artdaq_Fragment_.sequenceID());
	TRACE(4, "ContainerFragmentLoader::addFragment, copying %llu bytes from %p to %p", (long long unsigned int)frag.sizeBytes(), (void*)frag.headerAddress(), dataEnd_());
	memcpy(dataEnd_(), frag.headerAddress(), frag.sizeBytes());
	metadata()->index[block_count()] = lastFragmentIndex() + frag.sizeBytes();
	metadata()->block_count++;
}

inline void artdaq::ContainerFragmentLoader::addFragment(artdaq::FragmentPtr& frag)
{
	addFragment(*frag);
}

inline void artdaq::ContainerFragmentLoader::addFragments(artdaq::FragmentPtrs& frags)
{
	for (auto& frag : frags)
	{
		addFragment((*frag));
	}
}

#endif /* artdaq_core_Data_ContainerFragmentLoader_hh */
