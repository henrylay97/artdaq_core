if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

  cet_test(SharedMemoryManager_t USE_BOOST_UNIT
    LIBRARIES PRIVATE
    artdaq-core_Core
    artdaq-core_Utilities
    cetlib::headers
    cetlib_except::cetlib_except
  )
  cet_test(SharedMemoryFragmentManager_t USE_BOOST_UNIT
    LIBRARIES PRIVATE
    artdaq-core_Core 
    artdaq-core_Data
    artdaq-core_Utilities
    cetlib::headers
  )

endif()
