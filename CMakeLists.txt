cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

project(Justly VERSION 0.8.1 LANGUAGES CXX)

# download https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf2
# and put it into the share folder
# too big to commit to git

include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Widgets REQUIRED)

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        include-what-you-use
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
    )
endif()

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall" "-Wextra" "-Wpedantic" "$<$<CONFIG:Debug>:-Og>")
endif()

if (TRACK_COVERAGE)
    add_compile_options("--coverage")
    add_link_options("--coverage")
endif()

qt_add_library(JustlyLibrary INTERFACE)
qt_add_executable(Justly)
if (BUILD_TESTS)
   qt_add_executable(JustlyTests)
endif()

add_subdirectory("library")
add_subdirectory("executable")

install(TARGETS Justly)

if (WIN32)
    set(MAYBE_D "$<$<CONFIG:Debug>:d>")

    set(VCPKG_BIN_FOLDER "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin")

    install(FILES
        "${VCPKG_BIN_FOLDER}/glib-2.0-0.dll"
        "${VCPKG_BIN_FOLDER}/iconv-2.dll"
        "${VCPKG_BIN_FOLDER}/intl-8.dll"
        "$<TARGET_FILE:FluidSynth::libfluidsynth>"
        "$<TARGET_FILE:LibXml2::LibXml2>"
        "${VCPKG_BIN_FOLDER}/pcre2-16${MAYBE_D}.dll"
        "${VCPKG_BIN_FOLDER}/pcre2-8${MAYBE_D}.dll"
        "$<TARGET_FILE:Qt6::Core>"
        "$<TARGET_FILE:Qt6::Gui>"
        "$<TARGET_FILE:Qt6::Svg>"
        "$<TARGET_FILE:Qt6::Test>"
        "$<TARGET_FILE:Qt6::Widgets>"
        "${VCPKG_BIN_FOLDER}/zlib${MAYBE_D}1.dll"
        TYPE BIN
    )

    install(FILES "share/qt.conf" TYPE BIN)
    
    install(FILES
        "$<TARGET_FILE:Qt6::QSvgPlugin>"
        DESTINATION "plugins/imageformats"
    )

    install(FILES
        "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
        DESTINATION "plugins/platforms"
    )

    install(FILES
        "$<TARGET_FILE:Qt6::QWindowsVistaStylePlugin>"
        DESTINATION "plugins/styles"
    )   
endif()

install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)

if (BUILD_TESTS)
    add_subdirectory("tests")

    install(TARGETS JustlyTests)

    install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/" TYPE DATA)

    set(test_install_folder "${Justly_SOURCE_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    add_test(NAME run_tests COMMAND "${test_install_folder}/bin/JustlyTests")
endif()

include(CPack)