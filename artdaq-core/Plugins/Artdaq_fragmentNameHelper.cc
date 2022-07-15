#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Plugins/ArtdaqFragmentNameHelper.hh"

namespace artdaq {

ArtdaqFragmentNameHelper::ArtdaqFragmentNameHelper(std::string unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes)
    : FragmentNameHelper(unidentified_instance_name, extraTypes)
{
	TLOG(TLVL_DEBUG + 32) << "ArtdaqFragmentNameHelper CONSTRUCTOR START";
	TLOG(TLVL_DEBUG + 32) << "ArtdaqFragmentNameHelper CONSTRUCTOR END";
}

ArtdaqFragmentNameHelper::~ArtdaqFragmentNameHelper() = default;

}  // namespace artdaq

DEFINE_ARTDAQ_FRAGMENT_NAME_HELPER(artdaq::ArtdaqFragmentNameHelper)