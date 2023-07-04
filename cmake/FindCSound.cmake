# Try to find the Csound library.
# Once done this will define:
#  CSound::csound64 - main library
#  CSound::csnd6 - C++ bindings

include(FindPackageHandleStandardArgs)

if(APPLE)
  set(APPLE_PREFIX "/Library/Frameworks/CsoundLib64.framework")
  set(APPLE_LOCAL_PREFIX "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework")
  find_path(CSound_csound64_INCLUDE_DIR "csound/csound.h" HINTS
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSound_csound64_LIBRARY NAMES CsoundLib64 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  find_library(CSound_portaudio_LIBRARY NAMES libportaudio.so.2 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  find_path(CSound_csnd6_INCLUDE_DIR "csound/csound.hpp" HINTS 
    "${APPLE_PREFIX}/Headers"
    "${APPLE_LOCAL_PREFIX}/Headers"
  )
  find_library(CSound_csnd6_LIBRARY NAMES csnd6 HINTS
    APPLE_PREFIX
    APPLE_LOCAL_PREFIX
  )
  mark_as_advanced(APPLE_PREFIX APPLE_LOCAL_PREFIX)

  find_package_handle_standard_args(CSound
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR 
    CSound_csound64_LIBRARY 
    CSound_csnd6_LIBRARY
    CSound_portaudio_LIBRARY
  )
  mark_as_advanced(
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR 
    CSound_csound64_LIBRARY 
    CSound_csnd6_LIBRARY
    CSound_portaudio_LIBRARY
  )

  if(CSound_FOUND AND NOT TARGET CSound::portaudio)
    add_library(CSound::portaudio SHARED IMPORTED)
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_LIBRARY}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csound64)
    add_library(CSound::csound64 UNKNOWN IMPORTED)
    target_link_libraries(CSound::csound64 INTERFACE CSound::portaudio)
    target_include_directories(CSound::csound64 INTERFACE "${CSound_csound64_INCLUDE_DIR}")
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CSound_csound64_INCLUDE_DIR}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csnd6)
    add_library(CSound::csnd6 UNKNOWN IMPORTED)
    target_include_directories(CSound::csnd6 INTERFACE "${CSound_csnd6_INCLUDE_DIR}")
    set_target_properties(CSound::csnd6 PROPERTIES
      IMPORTED_LOCATION "${CSound_csnd6_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CSound_csnd6_INCLUDE_DIR}"
    )
  endif()
elseif(WIN32)
  set(WINDOWS_PREFIX "C:\\Program Files\\Csound6_x64")
  find_path(CSound_csound64_INCLUDE_DIR "csound/csound.h" HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_program(CSound_csound64_DLL "csound64.dll" HINTS
    "${WINDOWS_PREFIX}\\bin"
  )
  find_program(CSound_portaudio_DLL "portaudio.dll" HINTS
    "${WINDOWS_PREFIX}\\bin"
  )
  find_library(CSound_csound64_LIB NAMES csound64 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  find_path(CSound_csnd6_INCLUDE_DIR "csound/csound.hpp" HINTS
    "${WINDOWS_PREFIX}\\include"
  )
  find_program(CSound_csnd6_DLL "csnd6.dll" HINTS
    "${WINDOWS_PREFIX}\\bin"
  )
  find_library(CSound_csnd6_LIB NAMES csnd6 HINTS
    "${WINDOWS_PREFIX}\\lib"
  )
  mark_as_advanced(WINDOWS_PREFIX)
  find_package_handle_standard_args(CSound
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR 
    CSound_csound64_LIB
    CSound_csnd6_LIB
    CSound_csnd6_DLL
    CSound_csound64_DLL
    CSound_portaudio_DLL 
  )
  mark_as_advanced(
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR
    CSound_csound64_LIB
    CSound_csnd6_LIB
    CSound_csnd6_DLL
    CSound_csound64_DLL
    CSound_portaudio_DLL 
  )

  if(CSound_FOUND AND NOT TARGET CSound::portaudio)
    add_library(CSound::portaudio SHARED IMPORTED)
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_DLL}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csound64)
    add_library(CSound::csound64 SHARED IMPORTED)
    target_link_libraries(CSound::csound64 INTERFACE CSound::portaudio)
    target_include_directories(CSound::csound64 INTERFACE "${CSound_csound64_INCLUDE_DIR}")
    set_target_properties(CSound::csound64 PROPERTIES
      IMPORTED_LOCATION "${CSound_csound64_DLL}"
      IMPORTED_IMPLIB "${CSound_csound64_LIB}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csnd6)
    add_library(CSound::csnd6 SHARED IMPORTED)
    target_include_directories(CSound::csnd6 INTERFACE "${CSound_csnd6_INCLUDE_DIR}")
    set_target_properties(CSound::csnd6 PROPERTIES
      IMPORTED_LOCATION "${CSound_csnd6_DLL}"
      IMPORTED_IMPLIB "${CSound_csnd6_LIB}"
    )
  endif()
else()
  find_path(CSound_csound64_INCLUDE_DIR "csound/csound.h")
  find_library(CSound_csound64_LIBRARY NAMES csound64 csound)
  find_path(CSound_csnd6_INCLUDE_DIR "csound/csound.hpp")
  find_library(CSound_csnd6_LIBRARY NAMES csnd6)
  find_library(CSound_portaudio_LIBRARY NAMES libportaudio.so.2)

  find_package_handle_standard_args(CSound
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR 
    CSound_csound64_LIBRARY 
    CSound_csnd6_LIBRARY
    CSound_portaudio_LIBRARY
  )
  mark_as_advanced(
    CSound_csound64_INCLUDE_DIR 
    CSound_csnd6_INCLUDE_DIR 
    CSound_csound64_LIBRARY 
    CSound_csnd6_LIBRARY
    CSound_portaudio_LIBRARY
  )

  if(CSound_FOUND AND NOT TARGET CSound::portaudio)
    add_library(CSound::portaudio UNKNOWN IMPORTED)
    set_target_properties(CSound::portaudio PROPERTIES
      IMPORTED_LOCATION "${CSound_portaudio_LIBRARY}"
    )
  endif()

  if(CSound_FOUND AND NOT TARGET CSound::csound64)
    add_library(CSound::csound64 UNKNOWN IMPORTED)
    target_include_directories(CSound::csound64 INTERFACE "${CSound_csound64_INCLUDE_DIR}")
    target_link_libraries(CSound::csound64 INTERFACE CSound::portaudio)
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
endif()

# TODO: make sure we ship portaudio on all platforms