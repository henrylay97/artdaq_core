
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")

cet_test(SharedMemoryManager_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Core
  )
cet_test(SharedMemoryFragmentManager_t USE_BOOST_UNIT
  LIBRARIES artdaq-core_Core 
  artdaq-core_Data
  )

  endif()