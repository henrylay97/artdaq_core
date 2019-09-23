#ifndef artdaq_core_Generators_makeFragmentGenerator_hh
#define artdaq_core_Generators_makeFragmentGenerator_hh
// Using LibraryManager, find the correct library and return an instance
// of the specified generator.

#include "fhiclcpp/fwd.h"

#include <memory>
#include <string>

namespace artdaq {
class FragmentGenerator;

/**
   * \brief Instantiates the FragmentGenerator plugin with the given name, using the given ParameterSet
   * \param generator_plugin_spec Name of the Generator plugin (omit _generator.so)
   * \param ps The ParameterSet used to initialize the FragmentGenerator
   * \return A smart pointer to the FragmentGenerator instance
   */
std::unique_ptr<FragmentGenerator>
makeFragmentGenerator(std::string const& generator_plugin_spec,
                      fhicl::ParameterSet const& ps);
}  // namespace artdaq
#endif /* artdaq_core_Generators_makeFragmentGenerator_hh */
