
cet_make(
	EXCLUDE GenFile_mfPlugin.cc ExceptionStackTrace.cc
  LIBRARIES
  MF_MessageLogger
  cetlib
  cetlib_except
  fhiclcpp
	${EXCEPTIONSTACKTRACE_LIB}
	Boost::filesystem
)

cet_make_library(
	LIBRARY_NAME  artdaq-core_Utilities_ExceptionStackTrace
	SOURCE 
	ExceptionStackTrace.cc 
  LIBRARIES
	-ldl
)

basic_plugin( "GenFile" "mfPlugin"
  MF_MessageLogger
  cetlib
  cetlib_except
  fhiclcpp
)

install_headers()
install_source()
