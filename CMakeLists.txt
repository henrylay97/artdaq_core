# ======================================================================
#  artdaq-core main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq-core/ups/setup_for_development
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -Dqualifier:STRING=$CETPKG_QUAL
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package
# ======================================================================

cmake_minimum_required (VERSION 3.16)

find_package(cetmodules)
set(CET_CETBUILDTOOLS_COMPAT TRUE)
include(Compatibility)
set(CET_WARN_DEPRECATED)

project(artdaq_core VERSION 3.08.00)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS VIGILANT
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic
)

# these are minimum required versions, not the actual product versions
find_ups_boost(v1_53_0)
find_ups_product( canvas v1_00_00 )
find_ups_product( cetlib v1_08_00 )
find_ups_product( cetpkgsupport v1_08_03 )
find_ups_product( clhep v2_2_0 )
find_ups_product( fhiclcpp v3_04_00 )
find_ups_product( messagefacility v1_14_01 )
find_ups_product( sqlite v3_08_05_00 )
find_ups_product( tbb )
find_ups_root( v5_34_20 )
find_ups_product( TRACE v3_03_00 )


string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" MESSAGEFACILITY_VERSION_LIST "$ENV{MESSAGEFACILITY_VERSION}")
list(GET MESSAGEFACILITY_VERSION_LIST 0 MESSAGEFACILITY_MAJOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 1 MESSAGEFACILITY_MINOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 2 MESSAGEFACILITY_PATCH_VERSION)

message("-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION}")
add_definitions(-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION})

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" CANVAS_VERSION_LIST "$ENV{CANVAS_VERSION}")
list(GET CANVAS_VERSION_LIST 0 CANVAS_MAJOR_VERSION)
list(GET CANVAS_VERSION_LIST 1 CANVAS_MINOR_VERSION)
list(GET CANVAS_VERSION_LIST 2 CANVAS_PATCH_VERSION)

message("-DCANVAS_HEX_VERSION=0x${CANVAS_MAJOR_VERSION}${CANVAS_MINOR_VERSION}${CANVAS_PATCH_VERSION}")
add_definitions(-DCANVAS_HEX_VERSION=0x${CANVAS_MAJOR_VERSION}${CANVAS_MINOR_VERSION}${CANVAS_PATCH_VERSION})

# as of fhiclcpp v4_15_00 (s108), ParameterSet::make is the preferred way to make a ParameterSet
check_ups_version(fhiclcpp $ENV{FHICLCPP_VERSION} v4_15_00 PRODUCT_MATCHES_VAR PSET_MAKE)
if(PSET_MAKE)
  add_definitions(-DSIMPLER_PSET_MAKE)
endif()

# Debug streamer.
string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC)
if(BTYPE_UC STREQUAL DEBUG)
  add_definitions(-DDEBUGME)
	if(DEFINED ENV{USE_ASAN})
		if($ENV{USE_ASAN} GREATER 0)
			message("!!! Address Sanitizer Activated!!!")
			add_compile_options(-fsanitize=address -Wno-unused-command-line-argument)
			add_link_options(-lasan)
		endif()
	endif()
	if(DEFINED ENV{USE_TSAN})
		if($ENV{USE_TSAN} GREATER 0)
			message("!!! Thread Sanitizer Activated!!!")
			add_compile_options(-fsanitize=thread -Wno-unused-command-line-argument)
			add_link_options(-ltsan)
		endif()
	endif()
	if(DEFINED ENV{USE_GCOV})
		add_compile_options(-fprofile-arcs -ftest-coverage --coverage)
		add_link_options(--coverage)
	endif()
endif()

#exeption stack trace
if(DEFINED ENV{USE_EXCEPTIONSTACKTRACE})	
	if($ENV{USE_EXCEPTIONSTACKTRACE} GREATER 0)
		message("!!! Exception StackTrace Activated!!!")
		add_definitions(-D EXCEPTIONSTACKTRACE)
		set( EXCEPTIONSTACKTRACE_LIB  artdaq-core_Utilities_ExceptionStackTrace )
		add_compile_options( -rdynamic -Wno-unused-command-line-argument )
		add_link_options( -rdynamic )
	endif()
endif()

#cet_report_compiler_flags()

# macros for art_dictionary and simple_plugin
include(ArtDictionary)
include(CetMake)
include(BasicPlugin)

# testing
add_subdirectory(test)

# source
add_subdirectory(artdaq-core)

# doc - Documentation
if ( NOT DEFINED ENV{DISABLE_DOXYGEN} )
add_subdirectory(doc)
endif()

cet_cmake_config()
