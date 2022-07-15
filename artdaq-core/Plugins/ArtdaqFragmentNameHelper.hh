#ifndef _artdaq_core_Plugins_ArtdaqFragmentNameHelper_hh_
#define _artdaq_core_Plugins_ArtdaqFragmentNameHelper_hh_

#include <set>
#include <string>
#include <vector>

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Plugins/FragmentNameHelper.hh"

namespace artdaq {
/**
 * @brief Default implementation of FragmentNameHelper
 */
class ArtdaqFragmentNameHelper : public FragmentNameHelper
{
public:
	/**
	 * \brief DefaultArtdaqFragmentNameHelper Destructor
	 */
	virtual ~ArtdaqFragmentNameHelper();

	/**
	 * @brief ArtdaqFragmentNameHelper Constructor
	 * @param unidentified_instance_name Name to use for unidentified Fragments
	 * @param extraTypes Additional types to register
	 */
	ArtdaqFragmentNameHelper(std::string unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes);

private:
	ArtdaqFragmentNameHelper(ArtdaqFragmentNameHelper const&) = delete;
	ArtdaqFragmentNameHelper(ArtdaqFragmentNameHelper&&) = delete;
	ArtdaqFragmentNameHelper& operator=(ArtdaqFragmentNameHelper const&) = delete;
	ArtdaqFragmentNameHelper& operator=(ArtdaqFragmentNameHelper&&) = delete;
};
}  // namespace artdaq

#endif  //_artdaq_core_Plugins_ArtdaqFragmentNameHelper_hh_