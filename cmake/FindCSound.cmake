# Try to find the Csound library.
# Once done this will define:
#  CSound::csound64 - main library
#  CSound::csnd6 - C++ bindings

include(FindPackageHandleStandardArgs)

if(APPLE)
  # TODO
elseif(WIN32)
  find_path(CSound_ROOT_DIR "bin" HINTS
    "C:\\Program Files\\Csound6_x64"
  )
  
  find_path(CSound_csound64_INCLUDE_DIR "csound/csound.h" HINTS
    "${CSound_ROOT_DIR}\\include"
  )
  find_program(CSound_csound64_LIB "csound64.dll" HINTS
    "${CSound_ROOT_DIR}\\bin"
  )
  find_library(CSound_csound64_IMPLIB NAMES csound64 HINTS
    "${CSound_ROOT_DIR}\\lib"
  )

  find_path(CSound_csnd6_INCLUDE_DIR "csound/csound.hpp" HINTS
    "${CSound_ROOT_DIR}\\include"
  )
  find_program(CSound_csnd6_LIB "csnd6.dll" HINTS
    "${CSound_ROOT_DIR}\\bin"
  )
  find_library(CSound_csnd6_IMPLIB NAMES csnd6 HINTS
    "${CSound_ROOT_DIR}\\lib"
  )

  find_program(CSound_rtaudio_LIB "rtpa.dll" HINTS
    "${CSound_ROOT_DIR}\\plugins64"
  )
  
  mark_as_advanced(WINDOWS_PREFIX)
  find_package_handle_standard_args(CSound
    CSound_ROOT_DIR
    CSound_csound64_INCLUDE_DIR
    CSound_csound64_LIB
    CSound_csound64_IMPLIB
    CSound_csnd6_INCLUDE_DIR 
    CSound_csnd6_LIB
    CSound_csnd6_IMPLIB
    CSound_rtaudio_LIB
  )
  mark_as_advanced(
    CSound_ROOT_DIR
    CSound_csound64_INCLUDE_DIR
    CSound_csound64_LIB
    CSound_csound64_IMPLIB
    CSound_csnd6_INCLUDE_DIR 
    CSound_csnd6_LIB
    CSound_csnd6_IMPLIB
    CSound_rtaudio_LIB
  )

  if(CSound_FOUND AND NOT TARGET CSound::csound64)
    add_library(CSound::csound64 SHARED IMPORTED)
    target_include_directories(CSound::csound64 INTERFACE "${CSound_csound64_INCLUDE_DIR}")
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_LIB}"
      IMPORTED_IMPLIB "${CSound_csound64_IMPLIB}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csnd6)
    add_library(CSound::csnd6 SHARED IMPORTED)
    target_include_directories(CSound::csnd6 INTERFACE "${CSound_csnd6_INCLUDE_DIR}")
    set_target_properties(CSound::csnd6 PROPERTIES
      IMPORTED_LOCATION "${CSound_csnd6_LIB}"
      IMPORTED_IMPLIB "${CSound_csnd6_IMPLIB}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::rtaudio)
    add_library(CSound::rtaudio SHARED IMPORTED)
    set_target_properties(CSound::rtaudio PROPERTIES
      IMPORTED_LOCATION "${CSound_rtaudio_LIB}"
    )
  endif()
else()
  find_path(CSound_csound64_INCLUDE_DIR "csound/csound.h")
  find_library(CSound_csound64_LIBRARY NAMES csound64)
  
  find_path(CSound_csnd6_INCLUDE_DIR "csound/csound.hpp")
  find_library(CSound_csnd6_LIBRARY NAMES csnd6)

  cmake_path(GET CSound_csound64_LIBRARY PARENT_PATH CSound_LIBRARY_DIR)
  find_library(CSound_rtaudio_LIBRARY NAMES rtpulse HINTS
    "${CSound_LIBRARY_DIR}/csound/plugins64-6.0"
  )

  find_package_handle_standard_args(CSound
    CSound_csound64_INCLUDE_DIR
    CSound_csound64_LIBRARY 
    CSound_csnd6_INCLUDE_DIR
    CSound_csnd6_LIBRARY
    CSound_rtaudio_LIBRARY
  )
  mark_as_advanced(
    CSound_LIBRARY_DIR
    CSound_csound64_INCLUDE_DIR
    CSound_csound64_LIBRARY 
    CSound_csnd6_INCLUDE_DIR
    CSound_csnd6_LIBRARY
    CSound_rtaudio_LIBRARY
  )

  if(CSound_FOUND AND NOT TARGET CSound::csound64)
    add_library(CSound::csound64 UNKNOWN IMPORTED)
    target_include_directories(CSound::csound64 INTERFACE "${CSound_csound64_INCLUDE_DIR}")
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_LIBRARY}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csnd6)
    add_library(CSound::csnd6 UNKNOWN IMPORTED)
    target_include_directories(CSound::csnd6 INTERFACE "${CSound_csnd6_INCLUDE_DIR}")
    set_target_properties(CSound::csnd6 PROPERTIES
      IMPORTED_LOCATION "${CSound_csnd6_LIBRARY}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::rtaudio)
    add_library(CSound::rtaudio SHARED IMPORTED GLOBAL)
    set_target_properties(CSound::rtaudio PROPERTIES
      IMPORTED_LOCATION "${CSound_rtaudio_LIBRARY}"
    )
  endif()
endif()
