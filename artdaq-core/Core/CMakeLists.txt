# ======================================================================
#
# Build/install script
#
# ======================================================================

# ----------------------------------------------------------------------
# Build this project's library:

cet_make_library(SOURCE
  MonitoredQuantity.cc
  SharedMemoryEventReceiver.cc
  SharedMemoryFragmentManager.cc
  SharedMemoryManager.cc
  StatisticsCollection.cc
  LIBRARIES
  PUBLIC
	artdaq_core::artdaq-core_Data
  artdaq_core::artdaq-core_Utilities
	Boost::thread
  Threads::Threads
  PRIVATE
  artdaq_core::artdaq-core_Utilities_TraceLock
	cetlib_except::cetlib_except
  TRACE::TRACE
)

install_headers()
install_source()
