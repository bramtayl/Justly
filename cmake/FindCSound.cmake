# Try to find the Csound library.
# Once done this will define:
#  CSound::csound64 - main library
#  CSound::csnd6 - C++ bindings


if(APPLE)
  set(APPLE_PREFIX "/Library/Frameworks/CsoundLib64.framework")
  set(APPLE_LOCAL_PREFIX "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework")
  find_path(CSound_csound64_INCLUDE_DIR csound.h HINTS
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSound_csound64_LIBRARY NAMES CsoundLib64 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  find_path(CSound_csnd6_INCLUDE_DIR csound.hpp HINTS 
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSound_csnd6_LIBRARY NAMES csnd6 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  mark_as_advanced(APPLE_PREFIX APPLE_LOCAL_PREFIX)
elseif(WIN32)
  set(WINDOWS_PREFIX "C:\\Program Files\\Csound6_x64")
  find_path(CSound_csound64_INCLUDE_DIR csound.h PATH_SUFFIXES csound HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_library(CSound_csound64_LIBRARY NAMES csound64 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  find_path(CSound_csnd6_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_library(CSound_csnd6_LIBRARY NAMES csnd6 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  mark_as_advanced(WINDOWS_PREFIX)
else()
  find_path(CSound_csound64_INCLUDE_DIR csound.h PATH_SUFFIXES csound)
  find_library(CSound_csound64_LIBRARY NAMES csound64 csound)
  find_path(CSound_csnd6_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound)
  find_library(CSound_csnd6_LIBRARY NAMES csnd6)
endif()

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set CSound_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CSound
  CSound_csound64_INCLUDE_DIR 
  CSound_csnd6_INCLUDE_DIR 
  CSound_csound64_LIBRARY 
  CSound_csnd6_LIBRARY 
)
mark_as_advanced(
  CSound_csound64_INCLUDE_DIR 
  CSound_csnd6_INCLUDE_DIR 
  CSound_csound64_LIBRARY 
  CSound_csnd6_LIBRARY
)

if(CSound_FOUND AND NOT TARGET CSound::csound64)
  add_library(CSound::csound64 UNKNOWN IMPORTED)
  set_target_properties(CSound::csound64 PROPERTIES
    IMPORTED_LOCATION "${CSound_csound64_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CSound_csound64_INCLUDE_DIR}"
  )
endif()

if(CSound_FOUND AND NOT TARGET CSound::csnd6)
  add_library(CSound::csnd6 UNKNOWN IMPORTED)
  set_target_properties(CSound::csnd6 PROPERTIES
    IMPORTED_LOCATION "${CSound_csnd6_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CSound_csnd6_INCLUDE_DIR}"
  )
endif()
