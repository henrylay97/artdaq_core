cet_make(
NO_DICTIONARY
  LIBRARIES
  artdaq-core_Utilities
  ${CETLIB_LIBS}
  EXCLUDE Artdaq_fragmentNameHelper.cc
  )

basic_plugin(Artdaq "fragmentNameHelper" artdaq-core_Data)

cet_remove_compiler_flags(-fsanitize=address -lasan)
cet_remove_compiler_flags(-fsanitize=thread -ltsan)
  art_dictionary(DICTIONARY_LIBRARIES artdaq-core_Data)

install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
