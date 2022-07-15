
cet_make_library(
  SOURCE
  ExceptionHandler.cc
  SimpleLookupPolicy.cc
  TimeUtils.cc
  configureMessageFacility.cc
  LIBRARIES
  PUBLIC
	fhiclcpp::fhiclcpp
	cetlib::cetlib
  PRIVATE
  ${EXCEPTIONSTACKTRACE_LIB}
	messagefacility::MF_MessageLogger
	canvas::canvas
  cetlib_except::cetlib_except
	Boost::date_time
  Boost::filesystem
	TRACE::TRACE
)

cet_make_library(LIBRARY_NAME artdaq-core_Utilities_TraceLock INTERFACE
  SOURCE
  TraceLock.hh
  LIBRARIES INTERFACE
	TRACE::MF
  Threads::Threads
)

cet_make_library(
	LIBRARY_NAME artdaq-core_Utilities_ExceptionStackTrace
	SOURCE 
	ExceptionStackTrace.cc 
  LIBRARIES
  PUBLIC
  Threads::Threads
  PRIVATE
  TRACE::TRACE
  ${CMAKE_DL_LIBS}
)

include(mfPlugin)
cet_build_plugin(GenFile mfPlugin
  LIBRARIES PRIVATE
  messagefacility::MF_MessageLogger
  fhiclcpp::types
  fhiclcpp::fhiclcpp
  cetlib::cetlib
  Boost::date_time
  TRACE::TRACE
)

install_headers()
install_source()
