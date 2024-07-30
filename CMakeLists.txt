cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

project(Justly VERSION 0.5.7 LANGUAGES CXX)

# download https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf2
# and put it into the share folder
# too big to commit to git

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(DISABLE_AUDIO "Disable audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

# for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(FluidSynth CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        include-what-you-use
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
    )
endif()

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

if (TRACK_COVERAGE)
    add_compile_options(-Og)
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

# need to be explicit for Qt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

qt_add_library(JustlyLibrary SHARED)

# disable assertions for release
add_compile_definitions(JustlyLibrary PUBLIC $<$<CONFIG:Release>:QT_NO_DEBUG=1>)

if (DISABLE_AUDIO)
    target_compile_definitions(JustlyLibrary PRIVATE "DISABLE_AUDIO=1")
endif()

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(JustlyLibrary PRIVATE src)

# add source dir for build but not install interface
target_include_directories(JustlyLibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(JustlyLibrary PUBLIC
    FluidSynth::libfluidsynth
    nlohmann_json::nlohmann_json
    nlohmann_json_schema_validator::validator
    Qt6::Core
    Qt6::Widgets
)
target_link_libraries(JustlyLibrary PRIVATE Qt6::Gui)

# for dllexport instead of dllimport
target_compile_definitions(JustlyLibrary PRIVATE "JUSTLY_LIBRARY")

install(TARGETS JustlyLibrary
    RUNTIME_DEPENDENCY_SET runtime_dependencies
    FILE_SET justly_public_headers
)

if (WIN32)
    # on other platforms, these should be system libraries
    install(RUNTIME_DEPENDENCY_SET runtime_dependencies
        PRE_INCLUDE_REGEXES
            "glib"
            "iconv"
            "intl"
            "fluidsynth"
            "mp3lame"
            "mpg123"
            "nlohmann_json_schema_validator"
            "ogg"
            "opus"
            "pcre"
            "sndfile"
            "vorbis"
            "vorbisenc"
        PRE_EXCLUDE_REGEXES "."
        DIRECTORIES
            # vcpkg libraries
            "${_VCPKG_INSTALLED_DIR}/{VCPKG_TARGET_TRIPLET}/lib"
    )
    # not getting picked up as a runtime dependency?
    find_package(FLAC)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
endif()

function(qt_deploy executable_name)
    if (WIN32)
        qt_generate_deploy_app_script(TARGET ${executable_name}
            OUTPUT_SCRIPT script_file
        )
        install(SCRIPT ${script_file})
    endif()
endfunction()

qt_deploy(JustlyLibrary)
install(DIRECTORY ${Justly_SOURCE_DIR}/share/ TYPE DATA)

function(install_executable executable_name)
    install(TARGETS ${executable_name})

    qt_deploy(${executable_name})
    if (LINUX)
        set_target_properties(${executable_name} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")  
    endif()
    if (APPLE)
        set_target_properties(${executable_name} PROPERTIES INSTALL_RPATH "@loader_path/../lib")  
    endif()
endfunction()

qt_add_executable(Justly)
target_sources(Justly PRIVATE bin/main.cpp)
target_link_libraries(Justly PRIVATE JustlyLibrary Qt6::Widgets)
if (WIN32)
    # suppress console window
    set_target_properties(Justly PROPERTIES WIN32_EXECUTABLE ON)
endif()
install_executable(Justly)

if (BUILD_TESTS)
    qt_add_executable(JustlyTests)
    add_subdirectory(tests)
    target_include_directories(JustlyTests PRIVATE ${Justly_SOURCE_DIR})

    target_link_libraries(JustlyTests PRIVATE
        JustlyLibrary
        nlohmann_json::nlohmann_json
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Widgets
    )
    install_executable(JustlyTests)

    set(test_install_folder ${Justly_SOURCE_DIR}/test_install)

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )
    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    # test the installed app
    add_test(NAME run_tests COMMAND ${test_install_folder}/bin/JustlyTests)
endif()

include(CPack)