
cet_test(FragmentGenerator_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq_plugin_types::generator
  cetlib::headers
)
  
cet_test(FragmentNameHelper_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq_plugin_types::fragmentNameHelper
  cetlib::headers
)
