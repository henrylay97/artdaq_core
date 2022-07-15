cet_test(TimeUtils_t USE_BOOST_UNIT
	LIBRARIES PRIVATE
  artdaq-core_Utilities
  cetlib::headers
  TRACE::MF
)

cet_test(ExceptionHandler_t USE_BOOST_UNIT
	LIBRARIES PRIVATE
  artdaq-core_Utilities
	canvas::canvas
  cetlib::headers
	cetlib_except::cetlib_except
  TRACE::MF
)

cet_test(SimpleLookupPolicy_t USE_BOOST_UNIT
	DATAFILES fcl/LookupTarget.fcl
	LIBRARIES PRIVATE
  artdaq-core_Utilities
  cetlib::cetlib
  cetlib_except::cetlib_except
  TRACE::MF
  Boost::filesystem
)

cet_test(ExceptionStackTrace_t USE_BOOST_UNIT
	LIBRARIES PRIVATE
  artdaq-core_Utilities_ExceptionStackTrace
  cetlib::headers
  TRACE::MF
)

cet_test(configureMessageFacility_t USE_BOOST_UNIT
	LIBRARIES PRIVATE
  artdaq-core_Utilities
  fhiclcpp::fhiclcpp
  cetlib::headers
  cetlib_except::cetlib_except
  TRACE::MF
  Boost::filesystem
)

cet_test(GenFile_t USE_BOOST_UNIT
	LIBRARIES PRIVATE
  artdaq-core_Utilities
  messagefacility::MF_MessageLogger
  fhiclcpp::fhiclcpp
  cetlib::headers
  TRACE::MF
  Boost::filesystem
)
