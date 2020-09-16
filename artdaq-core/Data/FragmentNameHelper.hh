#ifndef _artdaq_core_Data_FragmentNameHelper_hh_
#define _artdaq_core_Data_FragmentNameHelper_hh_

#include <set>
#include <string>
#include <vector>

#include <cetlib/BasicPluginFactory.h>
#include <cetlib/compiler_macros.h>

#include "artdaq-core/Data/Fragment.hh"

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START \
	extern "C" {
#endif

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE DAQ Module
 */
#define DEFINE_ARTDAQ_FRAGMENT_NAME_HELPER(klass)                                                                                                   \
	EXTERN_C_FUNC_DECLARE_START                                                                                                                     \
	std::shared_ptr<artdaq::FragmentNameHelper> make(std::string unidentified, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> types) \
	{                                                                                                                                               \
		return std::shared_ptr<artdaq::FragmentNameHelper>(new klass(unidentified, types));                                                         \
	}                                                                                                                                               \
	}

namespace artdaq {
class FragmentNameHelper
{
public:
	/**
	 * \brief Default virtual destructor
	 */
	virtual ~FragmentNameHelper() = default;

	/**
	 * \brief FragmentNameHelper constructor
	 * \param ps ParameterSet used to configure FragmentNameHelper
	 *
	 * FragmentNameHelper accepts the following Parameters:
	 * "unidentified_instance_name" (Default: "unidentified"): Name to use for any Fragments which are not successfully translated by the FragmentNameHelper
	 * "fragment_type_map" (Default: []): A list of Fragment type_t to string pairs for additional types to register with the FragmentNameHelper
	 */
	FragmentNameHelper(std::string unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes)
	    : type_map_()
	    , unidentified_instance_name_(unidentified_instance_name)
	{
		SetBasicTypes(artdaq::Fragment::MakeSystemTypeMap());
		for (auto it = extraTypes.begin(); it != extraTypes.end(); ++it)
		{
			AddExtraType(it->first, it->second);
		}
	}

	/**
	 * \brief Sets the basic types to be translated.  (Should not include "container" types.)
	 */
	void SetBasicTypes(std::map<artdaq::Fragment::type_t, std::string> const& type_map)
	{
		for (auto& type_pair : type_map)
		{
			type_map_[type_pair.first] = type_pair.second;
		}
	}

	/**
	 * \brief Adds an additional type to be translated.
	 */
	void AddExtraType(artdaq::Fragment::type_t type_id, std::string const& type_name)
	{
		type_map_[type_id] = type_name;
	}

	/**
	 * \brief Get the configured unidentified_instance_name
	 * \return The configured unidentified_instance_name
	 */
	std::string GetUnidentifiedInstanceName() const { return unidentified_instance_name_; }

	/**
			 * \brief Returns the basic translation for the specified type. Must be implemented by derived classes
			 */
	virtual std::string GetInstanceNameForType(artdaq::Fragment::type_t type_id) const = 0;

	/**
			 * \brief Returns the full set of product instance names which may be present in the data, based on
			 *        the types that have been specified in the SetBasicTypes() and AddExtraType() methods.  This
			 *        *does* include "container" types, if the container type mapping is part of the basic types.
			 *  Must be implemented by derived classes
			 */
	virtual std::set<std::string> GetAllProductInstanceNames() const = 0;

	/**
			 * \brief Returns the product instance name for the specified fragment, based on the types that have
			 *        been specified in the SetBasicTypes() and AddExtraType() methods.  This *does* include the
			 *        use of "container" types, if the container type mapping is part of the basic types.  If no
			 *        mapping is found, the specified unidentified_instance_name is returned.
			 * Must be implemented by derived classes
			 */
	virtual std::pair<bool, std::string>
	GetInstanceNameForFragment(artdaq::Fragment const& fragment) const = 0;

protected:
	std::map<artdaq::Fragment::type_t, std::string> type_map_;  ///< Map relating Fragment Type to strings
	std::string unidentified_instance_name_;                    ///< The name to use for unknown Fragment types
private:
	FragmentNameHelper(FragmentNameHelper const&) = default;
	FragmentNameHelper(FragmentNameHelper&&) = default;
	FragmentNameHelper& operator=(FragmentNameHelper const&) = default;
	FragmentNameHelper& operator=(FragmentNameHelper&&) = default;
};

inline std::shared_ptr<FragmentNameHelper>
makeNameHelper(std::string const& plugin_name, std::string const& unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes)
{
	static cet::BasicPluginFactory bpf("fragmentNameHelper", "make");

	return bpf.makePlugin<std::shared_ptr<FragmentNameHelper>>(plugin_name, unidentified_instance_name, extraTypes);
}

class ArtdaqFragmentNameHelper : public FragmentNameHelper
{
public:
	/**
	 * \brief DefaultArtdaqFragmentNameHelper Destructor
	 */
	virtual ~ArtdaqFragmentNameHelper();

	/**
	 * \brief NetMonTransportService Constructor
	 * \param pset ParameterSet used to configure NetMonTransportService and DataSenderManager. See NetMonTransportService::Config
	 */
	ArtdaqFragmentNameHelper(std::string unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes);

	/**
			 * \brief Returns the basic translation for the specified type.  Defaults to the specified
			 *        unidentified_instance_name if no translation can be found.
			 */
	virtual std::string GetInstanceNameForType(artdaq::Fragment::type_t type_id) const;

	/**
			 * \brief Returns the full set of product instance names which may be present in the data, based on
			 *        the types that have been specified in the SetBasicTypes() and AddExtraType() methods.  This
			 *        *does* include "container" types, if the container type mapping is part of the basic types.
			 */
	virtual std::set<std::string> GetAllProductInstanceNames() const;

	/**
			 * \brief Returns the product instance name for the specified fragment, based on the types that have
			 *        been specified in the SetBasicTypes() and AddExtraType() methods.  This *does* include the
			 *        use of "container" types, if the container type mapping is part of the basic types.  If no
			 *        mapping is found, the specified unidentified_instance_name is returned.
			 */
	virtual std::pair<bool, std::string>
	GetInstanceNameForFragment(artdaq::Fragment const& fragment) const;

private:
	ArtdaqFragmentNameHelper(ArtdaqFragmentNameHelper const&) = delete;
	ArtdaqFragmentNameHelper(ArtdaqFragmentNameHelper&&) = delete;
	ArtdaqFragmentNameHelper& operator=(ArtdaqFragmentNameHelper const&) = delete;
	ArtdaqFragmentNameHelper& operator=(ArtdaqFragmentNameHelper&&) = delete;
};
}  // namespace artdaq

#endif  //_artdaq_core_Data_FragmentNameHelper_hh_