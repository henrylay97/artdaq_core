
#include "tracemf.h"
#define TRACE_NAME "GenFileOutput"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ProvideMakePluginMacros.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"
#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"

namespace mfplugins {
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility destination which generates the output file name based on some combination of
/// PID, hostname, application name, and/or timestamp
/// </summary>
class ELGenFileOutput : public ELdestination
{
public:
	/**
   * \brief Parameters used to configure GenFileOutput
   */
	struct Config
	{
		/// ELDestination common configuration parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// "append" (Default: true"): Whether to append to the file or recreate it
		fhicl::Atom<bool> append = fhicl::Atom<bool>{
		    fhicl::Name{"append"}, fhicl::Comment{"Whether to append to the file or recreate it"}, true};
		/// "directory" (Default: "/tmp"): The directory into which files will be saved
		fhicl::Atom<std::string> baseDir = fhicl::Atom<std::string>{
		    fhicl::Name{"directory"}, fhicl::Comment{"The directory into which files will be saved"}, "/tmp"};
		/// "seperator" (Default: "-"): Separator to use after optional replacement parameters
		fhicl::Atom<std::string> sep = fhicl::Atom<std::string>{
		    fhicl::Name{"seperator"}, fhicl::Comment{"Separator to use after optional replacement parameters"}, "-"};
		/// "timestamp_pattern" (Default: "%Y%m%d%H%M%S"): Pattern to use for %t strftime replacement
		fhicl::Atom<std::string> timePattern = fhicl::Atom<std::string>{
		    fhicl::Name{"timestamp_pattern"}, fhicl::Comment{"Pattern to use for %t strftime replacement"}, "%Y%m%d%H%M%S"};
		/**
     * \brief "pattern" (Default: "%N-%?H%t-%p.log"): Pattern to use for file naming.
     *
     * " Supported parameters are:\n"
     * %%: Print a % sign
     * %N: Print the executable name, as retrieved from /proc/$pid/exe
     * %?N: Print the executable name only if it does not already appear in the parsed format. Format is parsed left-to-right.
     * These options add a seperator AFTER if they are filled and if they are not the last token in the file pattern, before the last '.' character.
     * %H: Print the hostname, without any domain specifiers (i.e. work.fnal.gov will become work)
     * %?H: Print the hostname only if it does not already appear in the parsed format.
     * %p: Print the PID of the application configuring MessageFacility
     * %t: Print the timestamp using the format specified by timestamp_pattern
     * %T: Print the timestamp in ISO format
     */
		fhicl::Atom<std::string> filePattern = fhicl::Atom<std::string>{fhicl::Name{"pattern"}, fhicl::Comment{"Pattern to use for file naming.\n"
		                                                                                                       " Supported parameters are:\n"
		                                                                                                       " %%: Print a % sign\n"
		                                                                                                       " %N: Print the executable name, as retrieved from /proc/<pid>/exe\n"
		                                                                                                       " %?N: Print the executable name only if it does not already appear in the parsed format. "
		                                                                                                       "Format is parsed left-to-right.\n"
		                                                                                                       " These options add a seperator AFTER if they are filled and if they are not the last token in "
		                                                                                                       "the file pattern, before the last '.' character.\n"
		                                                                                                       " %H: Print the hostname, without any domain specifiers (i.e. work.fnal.gov will become work)\n"
		                                                                                                       " %?H: Print the hostname only if it does not already appear in the parsed format.\n"
		                                                                                                       " %p: Print the PID of the application configuring MessageFacility\n"
		                                                                                                       " %t: Print the timestamp using the format specified by timestamp_pattern\n"
		                                                                                                       " %T: Print the timestamp in ISO format"},
		                                                                "%N-%?H%t-%p.log"};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/**
		 * \brief ELGenFileOutput Constructor
		 * \param pset Validated ParameterSet used to configure GenFileOutput
		 */
	explicit ELGenFileOutput(Parameters const& pset);

	/**
		 * \brief Serialize a MessageFacility message to the output stream
		 * \param o Stringstream object containing message data
		 * \param e MessageFacility object containing header information
		 */
	void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

	/**
		 * \brief Flush any text in the ostream buffer to disk
		 */
	void flush() override;

private:
	std::unique_ptr<cet::ostream_handle> output_;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELGenFileOutput c'tor
//======================================================================

ELGenFileOutput::ELGenFileOutput(Parameters const& pset)
    : ELdestination(pset().elDestConfig())
{
	bool append = pset().append();
	std::string baseDir = pset().baseDir();
	std::string sep = pset().sep();
	std::string timePattern = pset().timePattern();
	std::string filePattern = pset().filePattern();

	auto pid = getpid();
	std::string exeString;
	std::string hostString;
	std::string timeBuffISO;  // Using boost::posix_time::to_iso_string (%T)
	std::string timeBuff;     // Using timestamp_pattern (%t)

	// Determine image name
	if (filePattern.find("%N") != std::string::npos || filePattern.find("%?N") != std::string::npos)
	{
		// get process name from '/proc/pid/exe'
		std::string exe;
		std::ostringstream pid_ostr;
		pid_ostr << "/proc/" << pid << "/exe";
		exe = std::string(realpath(pid_ostr.str().c_str(), nullptr));

		size_t end = exe.find('\0');
		size_t start = exe.find_last_of('/', end);
		exeString = exe.substr(start + 1, end - start - 1);
	}

	// Get Host name
	if (filePattern.find("%H") != std::string::npos || filePattern.find("%?H") != std::string::npos)
	{
		char hostname[256];
		if (gethostname(&hostname[0], 256) == 0)
		{
			std::string tmpString(hostname);
			hostString = tmpString;
			size_t pos = hostString.find('.');
			if (pos != std::string::npos && pos > 2)
			{
				hostString = hostString.substr(0, pos);
			}
		}
	}
	if (filePattern.find("%T") != std::string::npos)
	{
		timeBuffISO = boost::posix_time::to_iso_string(boost::posix_time::second_clock::universal_time());
	}
	if (filePattern.find("%t") != std::string::npos)
	{
		time_t rawtime;
		struct tm* timeinfo;
		char timeBuffC[256];
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(timeBuffC, 256, timePattern.c_str(), timeinfo);
		timeBuff = std::string(timeBuffC);
	}

	size_t pos = 0;
	TLOG(TLVL_DEBUG + 32) << "filePattern is: " << filePattern;
	while (filePattern.find('%', pos) != std::string::npos)
	{
		pos = filePattern.find('%', pos) + 1;
		TLOG(35, 1) << "Found % at " << (pos - 1) << ", next char: " << filePattern[pos] << ".";
		switch (filePattern[pos])
		{
			case '%':  // "%%"
				filePattern = filePattern.replace(pos - 1, 2, "%");
				pos--;
				break;
			case '?':
			{
				char next = filePattern[pos + 1];
				switch (next)
				{
					case 'N':
						if (filePattern.find(exeString) != std::string::npos)
						{
							filePattern = filePattern.erase(pos - 1, 3);
						}
						else
						{
							std::string repString = exeString;
							// Only append separator if we're not at the end of the pattern
							if (!(pos + 1 == filePattern.size() - 1 || pos + 2 == filePattern.find_last_of('.')))
							{
								repString += sep;
							}
							filePattern = filePattern.replace(pos - 1, 3, repString);
						}
						break;
					case 'H':
						if (filePattern.find(hostString) != std::string::npos)
						{
							filePattern = filePattern.erase(pos - 1, 3);
						}
						else
						{
							std::string repString = hostString;
							// Only append separator if we're not at the end of the pattern
							if (!(pos + 1 == filePattern.size() - 1 || pos + 2 == filePattern.find_last_of('.')))
							{
								repString += sep;
							}
							filePattern = filePattern.replace(pos - 1, 3, repString);
						}
						break;
					default:
						pos += 3;
						break;
				}
				pos -= 3;
			}
			break;
			case 'N':
				filePattern = filePattern.replace(pos - 1, 2, exeString);
				pos -= 2;
				break;
			case 'H':
				filePattern = filePattern.replace(pos - 1, 2, hostString);
				pos -= 2;
				break;
			case 'p':
				filePattern = filePattern.replace(pos - 1, 2, std::to_string(pid));
				pos -= 2;
				break;
			case 't':
				filePattern = filePattern.replace(pos - 1, 2, timeBuff);
				pos -= 2;
				break;
			case 'T':
				filePattern = filePattern.replace(pos - 1, 2, timeBuffISO);
				pos -= 2;
				break;
			default:
				break;
		}
		TLOG(TLVL_DEBUG + 35) << "filePattern is now: " << filePattern;
	}
	std::string fileName = baseDir + "/" + filePattern;
	TLOG(TLVL_DEBUG + 32) << "fileName is: " << fileName;

	output_ = std::make_unique<cet::ostream_handle>(fileName.c_str(), append ? std::ios::app : std::ios::trunc);
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELGenFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& /*msg*/)
{
	*output_ << oss.str();
	flush();
}

void ELGenFileOutput::flush()
{
	output_->flush();
}
}  // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

extern "C" {
MAKE_PLUGIN_START(auto, std::string const&, fhicl::ParameterSet const& pset)
{
	return std::make_unique<mfplugins::ELGenFileOutput>(pset);
}
MAKE_PLUGIN_END
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
