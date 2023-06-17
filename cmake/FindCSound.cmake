# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
  set(APPLE_PREFIX "/Library/Frameworks/CsoundLib64.framework")
  set(APPLE_LOCAL_PREFIX "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework")
  find_path(CSOUND_INCLUDE_DIR csound.h HINTS
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSOUND_LIBRARY NAMES CsoundLib64 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp HINTS 
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  find_library(CSOUND_STK_LIBRARY NAMES stkops HINTS
    "$ENV{HOME}/Library/csound/6.0/plugins64"
    "${APPLE_PREFIX}/Versions/6.0/Resources/Opcodes64"
    "${APPLE_LOCAL_PREFIX}/Versions/6.0/Resources/Opcodes64"
  )
  mark_as_advanced(APPLE_PREFIX APPLE_LOCAL_PREFIX)
elseif(WIN32)
  set(WINDOWS_PREFIX "C:\\Program Files\\Csound6_x64")
  find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_library(CSOUND_LIBRARY NAMES csound64 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  find_library(CSOUND_STK_LIBRARY NAMES stkops HINTS
    "C:\\Users\\$ENV{USERNAME}\\AppData\\Local\\csound\\6.0\\plugins64"
    "${WINDOWS_PREFIX}\\plugins64"
  )
  mark_as_advanced(WINDOWS_PREFIX)
else()
  find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound)
  find_library(CSOUND_LIBRARY NAMES csound64 csound)
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound)
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6)
  find_library(CSOUND_STK_LIBRARY NAMES stkops HINTS
    "/usr/lib/x86_64-linux-gnu/csound/plugins64-6.0/"
    "/usr/lib/csound/plugins64-6.0/"
    "/usr/local/lib/csound/plugins64-6.0/"
  )
endif()

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CSound
  CSOUND_INCLUDE_DIR 
  CSOUND_CPP_INCLUDE_DIR 
  CSOUND_LIBRARY 
  CSOUND_CPP_LIBRARY 
  CSOUND_STK_LIBRARY
)
mark_as_advanced(
  CSOUND_INCLUDE_DIR 
  CSOUND_CPP_INCLUDE_DIR 
  CSOUND_LIBRARY 
  CSOUND_CPP_LIBRARY 
  CSOUND_STK_LIBRARY
)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR} ${CSOUND_CPP_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} ${CSOUND_CPP_LIBRARY})
set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR} ${CSOUND_CPP_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} ${CSOUND_CPP_LIBRARY})