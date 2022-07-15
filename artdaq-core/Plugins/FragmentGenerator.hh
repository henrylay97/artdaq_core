#ifndef artdaq_core_Plugins_FragmentGenerator_hh
#define artdaq_core_Plugins_FragmentGenerator_hh

////////////////////////////////////////////////////////////////////////
// FragmentGenerator is an abstract class that defines the interface for
// obtaining events in artdaq. Subclasses are to override the (private) virtual
// functions; users of FragmentGenerator are to invoke the public
// (non-virtual) functions.
//
// getNext() will be called only from a single thread
////////////////////////////////////////////////////////////////////////

#include "artdaq-core/Data/Fragment.hh"

namespace artdaq {
/**
 * \brief Base class for all FragmentGenerators.
 *
 * FragmentGenerator is an abstract class that defines the interface for
 * obtaining events in artdaq. Subclasses are to override the (private) virtual
 * functions; users of FragmentGenerator are to invoke the public
 * (non-virtual) functions.
 */
class FragmentGenerator
{
public:
	/**
	 * \brief Default Constructor
	 */
	FragmentGenerator() = default;

	virtual ~FragmentGenerator() = default;

	/**
	 * \brief Obtain the next collection of Fragments.
	 * \param output New FragmentPtr objects will be added to this FragmentPtrs object.
	 * \return False indicates end-of-data
	 *
	 * Obtain the next collection of Fragments. Return false to indicate
	 * end-of-data. Fragments may or may not be in the same event;
	 * Fragments may or may not have the same FragmentID. Fragments
	 * will all be part of the same Run and SubRun.
	 */
	virtual bool getNext(PostmarkedFragmentPtrs& output) = 0;

	/**
	 * \brief Which fragment IDs does this FragmentGenerator generate?
	 * \return A std::vector of fragment_id_t
	 *
	 * Each FragmentGenerator is responsible for one or more Fragment IDs.
	 * Fragment IDs should be unique in an event, and consistent for a given piece of hardware.
	 */
	virtual std::vector<Fragment::fragment_id_t> fragmentIDs() = 0;

private:
	FragmentGenerator(FragmentGenerator const&) = delete;
	FragmentGenerator(FragmentGenerator&&) = delete;
	FragmentGenerator& operator=(FragmentGenerator const&) = delete;
	FragmentGenerator& operator=(FragmentGenerator&&) = delete;
};
}  // namespace artdaq

#endif /* artdaq_core_Plugins_FragmentGenerator_hh */
