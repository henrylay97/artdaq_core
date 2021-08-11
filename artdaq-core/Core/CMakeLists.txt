# ======================================================================
#
# Build/install script
#
# ======================================================================

# ----------------------------------------------------------------------
# Build this project's library:

cet_make(
  LIBRARIES
  artdaq-core_Data
  artdaq-core_Utilities
  pthread
  Boost::thread
  )

install_headers()
install_source()
