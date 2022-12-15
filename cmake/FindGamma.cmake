include(FindPackageHandleStandardArgs)

find_dependency(SndFile CONFIG REQUIRED)
find_dependency(portaudio CONFIG REQUIRED)

find_path(Gamma_INCLUDE_DIR "Gamma/Gamma.h")

function(is_debug result possible_path)
  if(NOT possible_path MATCHES "debug")
    set(${result} FALSE PARENT_SCOPE)
  endif()
endfunction()

find_library(Gamma_LIBRARY_DEBUG "gamma" VALIDATOR is_debug)

function(is_release result possible_path)
  if(possible_path MATCHES "debug")
    set(${result} FALSE PARENT_SCOPE)
  endif()
endfunction()

find_library(Gamma_LIBRARY_RELEASE "gamma" VALIDATOR is_release)

add_library(Gamma::gamma SHARED IMPORTED)
target_include_directories(Gamma::gamma INTERFACE "${Gamma_INCLUDE_DIR}")

# Interface so they don't propagate to dependencies
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_definitions(Gamma::gamma INTERFACE "NOMINMAX")
else()
  target_compile_definitions(Gamma::gamma INTERFACE "__STDC_CONSTANT_MACROS")
endif()

include(SelectLibraryConfigurations)
select_library_configurations(Gamma)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gamma
  FOUND_VAR Gamma_FOUND
  REQUIRED_VARS
    Gamma_LIBRARY
    Gamma_INCLUDE_DIR
)

set(CONFIGURATIONS "DEBUG" "RELEASE")

set(DEPENDENCIES SndFile::sndfile)
if (TARGET portaudio_static)
  list(APPEND DEPENDENCIES portaudio_static)
else()
  list(APPEND DEPENDENCIES portaudio)
endif()

set_target_properties(Gamma::gamma PROPERTIES
  IMPORTED_CONFIGURATIONS "${CONFIGURATIONS}"
  INTERFACE_LINK_LIBRARIES "${DEPENDENCIES}"
)

IF (WIN32)
  set_target_properties(Gamma::gamma PROPERTIES
    IMPORTED_IMPLIB_RELEASE "${Gamma_LIBRARY_RELEASE}"
    IMPORTED_IMPLIB_DEBUG "${Gamma_LIBRARY_DEBUG}"
  )
ELSE()
  set_target_properties(Gamma::gamma PROPERTIES
    IMPORTED_LOCATION_RELEASE "${Gamma_LIBRARY_RELEASE}"
    IMPORTED_LOCATION_DEBUG "${Gamma_LIBRARY_DEBUG}"
  )
ENDIF()
