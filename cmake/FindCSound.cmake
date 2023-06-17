# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
  find_path(CSOUND_INCLUDE_DIR csound.h HINTS /Library/Frameworks/CsoundLib64.framework/Headers
  "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers")
elseif(WIN32)
  find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound 
            HINTS "c:\\Program Files\\Csound6_x64\\include")
else()
  find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound)
endif()

if(APPLE)
  find_library(CSOUND_LIBRARY NAMES CsoundLib64 HINTS /Library/Frameworks/CsoundLib64.framework/
  "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework")
elseif(WIN32)
  find_library(CSOUND_LIBRARY NAMES csound64 HINTS "c:\\Program Files\\Csound6_x64\\lib")
else()
  find_library(CSOUND_LIBRARY NAMES csound64 csound)
endif()

if(APPLE)
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp HINTS /Library/Frameworks/CsoundLib64.framework/Headers
  "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers")
elseif(WIN32)
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound 
            HINTS "c:\\Program Files\\Csound6_x64\\include")
else()
  find_path(CSOUND_CPP_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound)
endif()


if(APPLE)
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6 HINTS /Library/Frameworks/CsoundLib64.framework/
  "$ENV{HOME}/Library/Frameworks/csnd6.framework")
elseif(WIN32)
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6 HINTS "c:\\Program Files\\Csound6_x64\\lib")
else()
  find_library(CSOUND_CPP_LIBRARY NAMES csnd6)
endif()


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CSound
  CSOUND_INCLUDE_DIR CSOUND_CPP_INCLUDE_DIR CSOUND_LIBRARY CSOUND_CPP_LIBRARY
)
mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY CSOUND_CPP_INCLUDE_DIR CSOUND_CPP_LIBRARY )

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR} ${CSOUND_CPP_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} ${CSOUND_CPP_LIBRARY})