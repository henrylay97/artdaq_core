#ifndef _artdaq_core_Plugins_FragmentNameHelper_hh_
#define _artdaq_core_Plugins_FragmentNameHelper_hh_

#include <set>
#include <string>
#include <vector>

#include <cetlib/BasicPluginFactory.h>
#include <cetlib/compiler_macros.h>

#include "artdaq-core/Data/ContainerFragment.hh"
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
/**
 * @brief The FragmentNameHelper translates between Fragments and their instance names (usually by type, but any/all RawFragmentHeader fields, or even Overlays, may be used)
 */
class FragmentNameHelper
{
public:
	/**
	 * \brief Default virtual destructor
	 */
	virtual ~FragmentNameHelper() = default;

	/**
	 * @brief FragmentNameHelper Constructor
	 * @param unidentified_instance_name Name to use for unidentified Fragments
	 * @param extraTypes Additional types to register
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
	virtual std::string GetInstanceNameForType(artdaq::Fragment::type_t type_id) const
	{
		if (type_map_.count(type_id) > 0) { return type_map_.at(type_id); }
		return unidentified_instance_name_;
	}

	/**
	 * \brief Returns the full set of product instance names which may be present in the data, based on
	 *        the types that have been specified in the SetBasicTypes() and AddExtraType() methods.  This
	 *        *does* include "container" types, if the container type mapping is part of the basic types.
	 *  Must be implemented by derived classes
	 */
	virtual std::set<std::string> GetAllProductInstanceNames() const
	{
		std::set<std::string> output;
		for (const auto& map_iter : type_map_)
		{
			std::string instance_name = map_iter.second;
			if (output.count(instance_name) == 0u)
			{
				output.insert(instance_name);
				TLOG(TLVL_DEBUG + 33) << "Adding product instance name \"" << map_iter.second << "\" to list of expected names";
			}
		}

		auto container_type = type_map_.find(artdaq::Fragment::type_t(artdaq::Fragment::ContainerFragmentType));
		if (container_type != type_map_.end())
		{
			std::string container_type_name = container_type->second;
			std::set<std::string> tmp_copy = output;
			for (const auto& set_iter : tmp_copy)
			{
				output.insert(container_type_name + set_iter);
			}
		}

		return output;
	}

	/**
	 * \brief Returns the product instance name for the specified fragment, based on the types that have
	 *        been specified in the SetBasicTypes() and AddExtraType() methods.  This *does* include the
	 *        use of "container" types, if the container type mapping is part of the basic types.  If no
	 *        mapping is found, the specified unidentified_instance_name should be returned.
	 * Must be implemented by derived classes
	 */
	virtual std::pair<bool, std::string>
	GetInstanceNameForFragment(artdaq::Fragment const& fragment) const
	{
		auto type_map_end = type_map_.end();
		bool success_code = true;
		std::string instance_name;

		auto primary_type = type_map_.find(fragment.type());
		if (primary_type != type_map_end)
		{
			TLOG(TLVL_DEBUG + 33) << "Found matching instance name " << primary_type->second << " for Fragment type " << static_cast<int>(fragment.type());
			instance_name = primary_type->second;
			if (fragment.type() == artdaq::Fragment::ContainerFragmentType)
			{
				artdaq::ContainerFragment cf(fragment);
				auto contained_type = type_map_.find(cf.fragment_type());
				if (contained_type != type_map_end)
				{
					instance_name += contained_type->second;
				}
			}
		}
		else
		{
			TLOG(TLVL_DEBUG + 33) << "Could not find match for Fragment type " << static_cast<int>(fragment.type()) << ", returning " << unidentified_instance_name_;
			instance_name = unidentified_instance_name_;
			success_code = false;
		}

		return std::make_pair(success_code, instance_name);
	}

protected:
	std::map<artdaq::Fragment::type_t, std::string> type_map_;  ///< Map relating Fragment Type to strings
	std::string unidentified_instance_name_;                    ///< The name to use for unknown Fragment types
private:
	FragmentNameHelper(FragmentNameHelper const&) = default;
	FragmentNameHelper(FragmentNameHelper&&) = default;
	FragmentNameHelper& operator=(FragmentNameHelper const&) = default;
	FragmentNameHelper& operator=(FragmentNameHelper&&) = default;
};

/**
 * @brief Create a FragmentNameHelper
 * @param plugin_name Name of the FragmentNameHelper plugin to load
 * @param unidentified_instance_name String to use for when the FragmentNameHelper cannot determine the Fragment name
 * @param extraTypes Additional types to register with the FragmentNameHelper
 * @return FragmentNameHelper shared_ptr handle
 */
inline std::shared_ptr<FragmentNameHelper>
makeNameHelper(std::string const& plugin_name, std::string const& unidentified_instance_name, std::vector<std::pair<artdaq::Fragment::type_t, std::string>> extraTypes)
{
	static cet::BasicPluginFactory bpf("fragmentNameHelper", "make");

	return bpf.makePlugin<std::shared_ptr<FragmentNameHelper>>(plugin_name, unidentified_instance_name, extraTypes);
}
}  // namespace artdaq

#endif  //_artdaq_core_Plugins_FragmentNameHelper_hh_