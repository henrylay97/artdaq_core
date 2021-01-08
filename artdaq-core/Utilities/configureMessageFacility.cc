#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <sstream>
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "cetlib_except/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"
#define TRACE_NAME "configureMessageFacility"
#include "tracemf.h"  // TRACE_CNTL, TRACE

namespace BFS = boost::filesystem;

std::string artdaq::generateMessageFacilityConfiguration(char const* progname, bool useConsole, bool printDebug, char const* fileExtraName)
{
	std::string logPathProblem;
	char* logRootString = getenv("ARTDAQ_LOG_ROOT");
	char* logFhiclCode = getenv("ARTDAQ_LOG_FHICL");
	char* artdaqMfextensionsDir = getenv("ARTDAQ_MFEXTENSIONS_DIR");
	char* useMFExtensionsS = getenv("ARTDAQ_MFEXTENSIONS_ENABLED");
	bool useMFExtensions = false;
	if (useMFExtensionsS != nullptr && !(strncmp(useMFExtensionsS, "0", 1) == 0))
	{
		useMFExtensions = true;
	}

	char* printTimestampsToConsoleS = getenv("ARTDAQ_LOG_TIMESTAMPS_TO_CONSOLE");
	bool printTimestampsToConsole = true;
	if (printTimestampsToConsoleS != nullptr && strncmp(printTimestampsToConsoleS, "0", 1) == 0)
	{
		printTimestampsToConsole = false;
	}

	std::string logfileDir;
	if (logRootString != nullptr)
	{
		if (!BFS::exists(logRootString))
		{
			logPathProblem = "Log file root directory ";
			logPathProblem.append(logRootString);
			logPathProblem.append(" does not exist!");
			throw cet::exception("ConfigureMessageFacility") << logPathProblem;  // NOLINT(cert-err60-cpp)
		}

		logfileDir = logRootString;
		logfileDir.append("/");
		logfileDir.append(progname);

		// As long as the top-level directory exists, I don't think we
		// really care if we have to create application directories...
		if (!BFS::exists(logfileDir))
		{
			BFS::create_directory(logfileDir);
			BFS::permissions(logfileDir, BFS::add_perms | BFS::owner_all | BFS::group_all | BFS::others_read);
		}
	}

	std::ostringstream ss;
	//ss << "debugModules:[\"*\"] "
	ss << "  destinations : { ";

	if (useConsole)
	{
		std::string outputLevel = "\"INFO\" ";
		if (printDebug)
		{
			outputLevel = "\"DEBUG\" ";
		}
		if (artdaqMfextensionsDir != nullptr && useMFExtensions)
		{
			ss << "    console : { "
			   << "      type : \"ANSI\" threshold : " << outputLevel;
			if (!printTimestampsToConsole)
			{
				ss << "      format: { timestamp: none } ";
			}
			ss << "      bell_on_error: true ";
			ss << "    } ";
		}
		else
		{
			ss << "    console : { "
			   << "      type : \"cout\" threshold :" << outputLevel;
			if (!printTimestampsToConsole)
			{
				ss << "       format: { timestamp: none } ";
			}
			ss << "    } ";
		}
	}

	if (!logfileDir.empty())
	{
		ss << " file: {";
		ss << R"( type: "GenFile" threshold: "DEBUG" seperator: "-")";
		ss << " pattern: \"" << progname << fileExtraName << "-%?H%t-%p.log"
		   << "\"";
		ss << " timestamp_pattern: \"%Y%m%d%H%M%S\"";
		ss << " directory: \"" << logfileDir << "\"";
		ss << " append : false";
		ss << " }";
	}

	if (artdaqMfextensionsDir != nullptr && useMFExtensions)
	{
		ss << "    trace : { "
		   << R"(       type : "TRACE" threshold : "DEBUG" format:{noLineBreaks: true} lvls: 0x7 lvlm: 0xF)"
		   << "    } ";
	}

	if (logFhiclCode != nullptr)
	{
		std::ifstream logfhicl(logFhiclCode);

		if (logfhicl.is_open())
		{
			std::stringstream fhiclstream;
			fhiclstream << logfhicl.rdbuf();
			ss << fhiclstream.str();
		}
		else
		{
			throw cet::exception("configureMessageFacility") << "Unable to open requested fhicl file \"" << logFhiclCode << "\".";  // NOLINT(cert-err60-cpp)
		}
	}

	ss << "  } ";

	std::string pstr(ss.str());

	//Canonicalize string:
	fhicl::ParameterSet tmp_pset;
	try
	{
		fhicl::make_ParameterSet(pstr, tmp_pset);
	}
	catch (cet::exception const& ex)
	{
		ExceptionHandler(ExceptionHandlerRethrow::yes, std::string("Exception occurred while processing fhicl ParameterSet string ") + pstr + ":");
	}
	return tmp_pset.to_string();
}
// generateMessageFacilityConfiguration

void artdaq::configureTRACE(fhicl::ParameterSet& trace_pset)
{
	/* The following code handles this example fhicl:
	   TRACE:{
		 TRACE_NUMENTS:500000
		 TRACE_ARGSMAX:10
		 TRACE_MSGMAX:0
		 TRACE_FILE:"/tmp/trace_buffer_%u"   # this is the default
		 TRACE_LIMIT_MS:[8,80,800]
		 TRACE_MODE:0xf
		 TRACE_NAMLVLSET:{
		   #name:[lvlsmskM,lvlsmskS[,lvlsmskT]]   lvlsmskT is optional
		   name0:[0x1f,0x7]
		   name1:[0x2f,0xf]
		   name2:[0x3f,0x7,0x1]
		 }
	   }
	*/
	std::vector<std::string> names = trace_pset.get_names();
	std::vector<std::string> trace_envs = {//"TRACE_NUMENTS", "TRACE_ARGSMAX", "TRACE_MSGMAX", "TRACE_FILE",
	                                       "TRACE_LIMIT_MS", "TRACE_MODE", "TRACE_NAMLVLSET"};
	std::unordered_map<std::string, bool> envs_set_to_unset;
	for (const auto& env : trace_envs)
	{
		envs_set_to_unset[env] = false;
	}
	// tricky - some env. vars. will over ride info in "mapped" (file) context while others cannot.
	for (const auto& name : names)
	{
		if (name == "TRACE_NUMENTS" || name == "TRACE_ARGSMAX" || name == "TRACE_MSGMAX" || name == "TRACE_FILE")
		{  // only applicable if env.var. set before before traceInit
			// don't override and don't "set_to_unset" (if "mapping", want any subprocess to map also)
			setenv(name.c_str(), trace_pset.get<std::string>(name).c_str(), 0);
			// These next 3 are looked at when TRACE_CNTL("namlvlset") is called. And, if mapped, get into file! (so may want to unset env???)
		}
		else if (name == "TRACE_LIMIT_MS")
		{  // there is also TRACE_CNTL
			if (getenv(name.c_str()) == nullptr)
			{
				envs_set_to_unset[name] = true;
				auto limit = trace_pset.get<std::vector<uint32_t>>(name);
				// could check that it is size()==3???
				std::string limits = std::to_string(limit[0]) + "," + std::to_string(limit[1]) + "," + std::to_string(limit[2]);
				setenv(name.c_str(), limits.c_str(), 0);
			}
		}
		else if (name == "TRACE_MODE")
		{  // env.var. only applicable if TRACE_NAMLVLSET is set, BUT could TRACE_CNTL("mode",mode)???
			if (getenv(name.c_str()) == nullptr)
			{
				envs_set_to_unset[name] = true;
				setenv(name.c_str(), trace_pset.get<std::string>(name).c_str(), 0);
			}
		}
		else if (name == "TRACE_NAMLVLSET")
		{
			if (getenv(name.c_str()) == nullptr)
			{
				envs_set_to_unset[name] = true;
				std::stringstream lvlsbldr;  // levels builder
				auto lvls_pset = trace_pset.get<fhicl::ParameterSet>(name);
				std::vector<std::string> tnames = lvls_pset.get_names();
				for (const auto& tname : tnames)
				{
					lvlsbldr << tname;
					auto msks = lvls_pset.get<std::vector<double>>(tname);
					for (auto msk : msks)
					{
						lvlsbldr << " 0x" << std::hex << static_cast<unsigned long long>(msk);  // NOLINT(google-runtime-int)
					}
					lvlsbldr << "\n";
				}
				setenv(name.c_str(), lvlsbldr.str().c_str(), 0);  // 0 means: won't overwrite
			}
		}
	}
	TRACE_CNTL("namlvlset");  // acts upon env.var.
	for (const auto& env : trace_envs)
	{
		if (envs_set_to_unset[env])
		{
			unsetenv(env.c_str());
		}
	}
}

void artdaq::configureMessageFacility(char const* progname, bool useConsole, bool printDebug)
{
	auto pstr = generateMessageFacilityConfiguration(progname, useConsole, printDebug);
	fhicl::ParameterSet pset;
	try
	{
		fhicl::make_ParameterSet(pstr, pset);
	}
	catch (cet::exception const&)
	{
		ExceptionHandler(ExceptionHandlerRethrow::yes, std::string("Exception occurred while processing fhicl ParameterSet string ") + pstr + ":");
	}

	fhicl::ParameterSet trace_pset;
	if (!pset.get_if_present<fhicl::ParameterSet>("TRACE", trace_pset))
	{
		fhicl::ParameterSet trace_dflt_pset;
		fhicl::make_ParameterSet("TRACE:{TRACE_MSGMAX:0 TRACE_LIMIT_MS:[10,500,1500]}", trace_dflt_pset);
		pset.put<fhicl::ParameterSet>("TRACE", trace_dflt_pset.get<fhicl::ParameterSet>("TRACE"));
		trace_pset = pset.get<fhicl::ParameterSet>("TRACE");
	}
	configureTRACE(trace_pset);
	pstr = pset.to_string();
	pset.erase("TRACE");

#if CANVAS_HEX_VERSION >= 0x30300  // art v2_11_00
	mf::StartMessageFacility(pset, progname);

#else
	mf::StartMessageFacility(pset);

	mf::SetApplicationName(progname);

	mf::setEnabledState("");
#endif
	TLOG(TLVL_TRACE) << "Message Facility Config input is: " << pstr;
	TLOG(TLVL_INFO) << "Message Facility Application " << progname << " configured with: " << pset.to_string();
}

std::string artdaq::setMsgFacAppName(const std::string& appType, unsigned short port)
{
	std::string appName(appType);

	char hostname[256];
	if (gethostname(&hostname[0], 256) == 0)
	{
		std::string hostString(hostname);
		size_t pos = hostString.find('.');
		if (pos != std::string::npos && pos > 2)
		{
			hostString = hostString.substr(0, pos);
		}
		appName.append("-");
		appName.append(hostString);
	}

	appName.append("-");
	appName.append(boost::lexical_cast<std::string>(port));

	mf::SetApplicationName(appName);
	return appName;
}
