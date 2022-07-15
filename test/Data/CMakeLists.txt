
cet_test(Fragment_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq-core_Data
  cetlib::headers
)

cet_test(ContainerFragment_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq-core_Data
  cetlib::headers
)

cet_test(RawEvent_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq-core_Data
  cetlib::headers
)

cet_test(BuildInfo_t USE_BOOST_UNIT
  LIBRARIES PRIVATE
  artdaq-core_Data
  cetlib::headers
)
