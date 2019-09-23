#ifndef artdaq_Application_configureMessageFacility_hh
#define artdaq_Application_configureMessageFacility_hh

#include <string>
#include "fhiclcpp/ParameterSet.h"

namespace artdaq {
/**
	* \brief Create the MessageFacility configuration Fhicl string
	* \param progname The name of the program
	* \param useConsole Should console output be activated? Default = true
	 * \param printDebug Whether Debug-level messages should be printed to console. Default = false
	* \return Fhicl string with generated MessageFacility configuration
	* \throw cet::exception if log path or ARTDAQ_LOG_FHICL do not exist
	*/
std::string generateMessageFacilityConfiguration(char const* progname, bool useConsole = true, bool printDebug = false);

/**
	 * \brief Configure TRACE
	 * \param trace_pset A fhicl::ParameterSet with the contents of the TRACE table
	 */
void configureTRACE(fhicl::ParameterSet& trace_pset);

/**
	 * \brief Configure and start the message facility. Provide the program name so that messages will be appropriately tagged.
	 * \param progname The name of the program
	 * \param useConsole Should console output be activated? Default = true
	 * \param printDebug Whether Debug-level messages should be printed to console. Default = false
	 */
void configureMessageFacility(char const* progname, bool useConsole = true, bool printDebug = false);

/**
	 * \brief Set the message facility application name using the specified application type and port number
	 * \param appType Application name
	 * \param port XMLRPC port of this application instance
	 * \return Name of the application as set for MessageFacility
	 */
std::string setMsgFacAppName(const std::string& appType, unsigned short port);
}  // namespace artdaq

#endif /* artdaq_Application_configureMessageFacility_hh */
