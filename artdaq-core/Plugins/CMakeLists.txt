include(BasicPlugin)

cet_register_export_set(SET_NAME PluginTypes NAMESPACE artdaq_plugin_types)
cet_register_export_set(SET_NAME PluginSupport NAMESPACE artdaq_plugin_support)
# Everything following without a specific EXPORT_SET defined must be in
# a new export set to avoid a circular dependence between export sets.
cet_register_export_set(SET_NAME PluginUsers SET_DEFAULT
  NAMESPACE ${${CETMODULES_CURRENT_PROJECT_NAME}_NAMESPACE})

cet_make_library(LIBRARY_NAME fragmentNameHelper INTERFACE
	EXPORT_SET PluginTypes
	SOURCE FragmentNameHelper.hh
	LIBRARIES INTERFACE
	artdaq_core::artdaq-core_Data
  cetlib::cetlib
  cetlib::compiler_macros
)

cet_write_plugin_builder(artdaq::fragmentNameHelper art::plugin Modules
  SUFFIX fragmentNameHelper
  INSTALL_BUILDER
  LIBRARIES CONDITIONAL artdaq_plugin_types::fragmentNameHelper
)

cet_make_library(LIBRARY_NAME generator INTERFACE
  EXPORT_SET PluginTypes
  SOURCE FragmentGenerator.hh
  LIBRARIES INTERFACE
	artdaq_core::artdaq-core_Data
)

cet_write_plugin_builder(artdaq::generator art::plugin Modules
  SUFFIX generator
  INSTALL_BUILDER
  LIBRARIES
  CONDITIONAL artdaq_plugin_types::generator
  REG cetlib::compiler_macros
)

cet_collect_plugin_builders(Modules ArtdaqCorePlugins LIST
  artdaq::fragmentNameHelper
  artdaq::generator
)

include(ArtdaqCorePlugins)

cet_build_plugin(Artdaq artdaq::fragmentNameHelper)

cet_make_library(SOURCE
  makeFragmentGenerator.cc
  LIBRARIES
  PRIVATE
  artdaq_plugin_types::generator
  cetlib::cetlib
)

cet_make_alias(NAME generatorMaker EXPORT_SET PluginSupport
  TARGET artdaq_core::artdaq-core_Plugins
)

install_headers()
install_source()
