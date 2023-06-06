# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

find_path(CSOUND_INCLUDE_DIR csound.hpp PATH_SUFFIXES csound REQUIRED)
find_library(CSOUND_LIBRARY NAMES csound64 PATH_SUFFIXES csound REQUIRED)
find_library(CSOUND_DEV_LIBRARY NAMES csnd6 PATH_SUFFIXES csound REQUIRED)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} ${CSOUND_DEV_LIBRARY} )