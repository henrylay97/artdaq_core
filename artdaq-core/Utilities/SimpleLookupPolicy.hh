#ifndef artdaq_core_Utilities_SimpleLookupPolicy_h
#define artdaq_core_Utilities_SimpleLookupPolicy_h

#include <memory>
#include "cetlib/filepath_maker.h"
#include "cetlib/search_path.h"

namespace artdaq {
class SimpleLookupPolicy;
}

/**
 * \brief This class is intended to find files using a set lookup order.
 *
 *   This class is intended to find files using the following lookup order:
 * - the absolute path, if provided
 * - the current directory
 * - the specified list of paths
 */
class artdaq::SimpleLookupPolicy : public cet::filepath_maker
{
public:
	/**
	 * \brief Flag if the constructor argument is a list of paths or the name of an environment variable
	 */
	enum class ArgType : int
	{
		ENV_VAR = 0,     ///< Constructor argument is environment variable name
		PATH_STRING = 1  ///< Constructor argument is a list of directories
	};

	/**
	 * \brief Constructor
	 * \param paths Either the name of an environment variable or a list of directories to search
	 * \param argType Flag to determine if paths argument is an environment variable or a list of directories
	 *
	 * The SimpleLookupPolicy Constructor instantiates the cet::search_path objects used for file lookup.
	 */
	SimpleLookupPolicy(std::string const& paths, ArgType argType = ArgType::ENV_VAR);

	/**
	 * \brief Perform the file lookup.
	 * \param filename The name of the file to find
	 * \return The location that the file was found in.
	 *
	 * The lookup proceeds in the following order:
	 * - the absolute path, if provided
	 * - the current directory
	 * - the specified list of paths
	 */
	std::string operator()(std::string const& filename) override;

	/**
	 * \brief Default destructor
	 */
	virtual ~SimpleLookupPolicy() noexcept;

private:
	SimpleLookupPolicy(SimpleLookupPolicy const&) = delete;
	SimpleLookupPolicy(SimpleLookupPolicy&&) = delete;
	SimpleLookupPolicy& operator=(SimpleLookupPolicy const&) = delete;
	SimpleLookupPolicy& operator=(SimpleLookupPolicy&&) = delete;

	/**
	 * \brief A cet::search_path object for the current directory
	 */
	std::unique_ptr<cet::search_path> cwdPath_;
	/**
	 * \brief A cet::search_path object for the paths specified in the constructor
	 */
	std::unique_ptr<cet::search_path> fallbackPaths_;
};

#endif /* artdaq_core_Utilities_SimpleLookupPolicy_h */

// Local Variables:
// mode: c++
// End:
