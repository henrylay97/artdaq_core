#include "artdaq-core/Utilities/SimpleLookupPolicy.hh"
#include "cetlib/filesystem.h"

artdaq::SimpleLookupPolicy::
    SimpleLookupPolicy(std::string const& paths, ArgType argType)
{
	// the cetlib search_path constructor expects either the name of
	// an environmental variable that contains the search path *or* a
	// colon-delimited list of paths.  So, a constructor argument of
	// a single path string is doomed to fail because it gets interpreted
	// as an env var.  So, in this class, we will always pass either
	// an env var name or a list of paths.  If/when a single path is
	// specified, we'll simply duplicate it so that search_path will
	// do the right thing.
	cwdPath_.reset(new cet::search_path(".:."));

	// if no fallback path was specified, simply use the current directory
	if (paths.empty())
	{
		fallbackPaths_.reset(new cet::search_path(".:."));
		return;
	}

	if (argType == ArgType::PATH_STRING)
	{
		auto workString(paths);
		if (workString.find(':') == std::string::npos)
		{
			workString.append(":");
			workString.append(paths);
		}
		fallbackPaths_.reset(new cet::search_path(workString));
	}

	else
	{  // argType == ENV_VAR
		fallbackPaths_.reset(new cet::search_path(paths));
	}
}

std::string artdaq::SimpleLookupPolicy::operator()(std::string const& filename)
{
	if (cet::is_absolute_filepath(filename))
	{
		return filename;
	}

	try
	{
		return cwdPath_->find_file(filename);
	}
	catch (...)
	{}

	return fallbackPaths_->find_file(filename);
}

artdaq::SimpleLookupPolicy::~SimpleLookupPolicy() noexcept {}
