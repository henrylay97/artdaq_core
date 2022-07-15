#ifndef artdaq_core_Utilities_ExceptionHandler_hh
#define artdaq_core_Utilities_ExceptionHandler_hh

#include <string>

namespace artdaq {
/**
 * \brief Controls whether the ExceptionHandler will rethrow after printing exception details
 */
enum class ExceptionHandlerRethrow
{
	yes,  ///< Rethrow the exception after sending details to MessageFacility
	no    ///< Consume the exception and proceed
};

/**
 * \brief The ExceptionHandler class prints out all available information about an excection, then
 * optionally re-throws.
 * \param decision Controls whether the ExceptionHandler will rethrow (ExceptionHandlerRethrow::yes) or not (ExceptionHandlerRethow::no)
 * \param optional_message An optional std::string giving more information about where the exception was originally caught
 *
 * JCF, 5/28/15
 *
 * The ExceptionHandler() function is designed to be called within a catch-all block:
 * ~~~~~~~~{.cpp}
 *     try {
 *         // ...Code that might throw an exception...
 *     } catch (...) {
 *            ExceptionHandler(artdaq::ExceptionHandlerRethrow::yes,
 *            "Optional string providing additional info");
 *     }
 * ~~~~~~~~
 *
 * Where above, you could switch out
 * `artdaq::ExceptionHandlerRethrow::yes` with
 * `artdaq::ExceptionHandlerRethrow::no`, depending on what you wish to do
 *
 * The details of ExceptionHandler() are as follows:
 *
 * - If an optional string is passed to it, use messagefacility to write the string with mf::LogError()
 *
 * - Apply a set of different catch-blocks to the original exception,
 * printing out as much information as possible contained within the
 * different exception types (art::Exception, cet::exception,
 * boost::exception and std::exception), again using mf::LogError()
 *
 * - If artdaq::ExceptionHandlerRethrow::yes was passed to
 * ExceptionHandler(), re-throw the exception rather than swallow it
 */
void ExceptionHandler(ExceptionHandlerRethrow decision, const std::string& optional_message = "");
}  // namespace artdaq

#endif
