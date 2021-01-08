
cet_test(Fragment_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Data
  )

cet_test(ContainerFragment_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Data
  ${MESSAGEFACILITY_LIBS}
  )

cet_test(RawEvent_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Data
  )

cet_test(BuildInfo_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Data
  )

cet_test(FragmentNameHelper_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Data
  )