cet_make_library(SOURCE
  Fragment.cc
  RawEvent.cc
  LIBRARIES
  PUBLIC
  artdaq_core::artdaq-core_Utilities
  cetlib_except::cetlib_except
  TRACE::MF
  TRACE::TRACE
)

cet_make_library(LIBRARY_NAME artdaq-core::Data_ParentageMap INTERFACE
  SOURCE detail/ParentageMap.hh
  LIBRARIES INTERFACE
  canvas::canvas
)

include(ArtDictionary)

art_dictionary(DICTIONARY_LIBRARIES
  artdaq-core_Data
  artdaq-core_Data_ParentageMap
)

get_target_property(DictCompileOptions artdaq-core_Data_dict COMPILE_OPTIONS)
if(DictCompileOptions)
	string(REPLACE "-fsanitize=thread" "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-fsanitize=address" "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-ltsan" "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-lasan" "" DictCompileOptions "${DictCompileOptions}")

	set_target_properties(artdaq-core_Data_dict PROPERTIES COMPILE_OPTIONS "${DictCompileOptions}")
endif()

install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
