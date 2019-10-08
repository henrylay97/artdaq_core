
find_library(LIBRT NAMES rt) 
if(LIBRT)
  set(FOUND_RT ${LIBRT})
endif()

cet_make(
	EXCLUDE GenFile_mfPlugin.cc
  LIBRARIES 
  ${MESSAGEFACILITY_LIBS}
  fhiclcpp
  ${CETLIB_LIBS}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${FOUND_RT}
)

basic_plugin( "GenFile" "mfPlugin"
   ${MESSAGEFACILITY_LIBS}
   ${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
)

install_headers()
install_source()
