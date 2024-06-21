cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.4.5.1 LANGUAGES CXX)

# requires at least qt 6.5.1
# newer than version currently shipped by ubuntu
# install with qt instead

# download https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf2
# and put it into the share folder
# too big to commit to git

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CPack)
include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" ON)
option(TRACK_COVERAGE "Track coverage" ON)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" ON)
option(CLANG_TIDY "Run clang-tidy" OFF)

# for tools to use
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# for libraries to reference other libraries in the same folder
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# only will work on my machine
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

find_package(FLAC)
find_package(FluidSynth CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)

qt_standard_project_setup()

# optionally run include-what-you-use to tidy includes
find_program(include_what_you_use_executable NAMES include-what-you-use)
if (INCLUDE_WHAT_YOU_USE AND NOT (include_what_you_use_executable STREQUAL "include_what_you_use_executable-NOTFOUND"))
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${include_what_you_use_executable}")
endif()

# optionally run clang-tidy to tidy code
find_program(clang_tidy_executable NAMES clang-tidy)
if (CLANG_TIDY AND NOT (clang_tidy_executable STREQUAL "clang_tidy_executable-NOTFOUND"))
    set(CMAKE_CXX_CLANG_TIDY ${clang_tidy_executable})
endif()

# add general warnings
if (MSVC)
    check_compiler_flag(CXX /W4 has_all_warnings)
    if (has_all_warnings)
        add_compile_options(/W4)
    endif()
else()
    check_compiler_flag(CXX -Wall has_all_warnings)
    if (has_all_warnings)
        add_compile_options(-Wall)
    endif()

    check_compiler_flag(CXX -Wextra has_extra_warnings)
    if (has_extra_warnings)
        add_compile_options(-Wextra)
    endif()
endif()

if (TRACK_COVERAGE)
    # debug optimization for better coverage stats
    check_compiler_flag(CXX -Og has_debug_optimization)
    if (has_debug_optimization)
        add_compile_options(-Og)
    endif()

    # requires matching link flag so set then unset
    set(CMAKE_REQUIRED_LINK_OPTIONS --coverage)
    check_compiler_flag(CXX --coverage has_coverage)
    set(CMAKE_REQUIRED_LINK_OPTIONS "")
    if (has_coverage)
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

qt_add_library(JustlyLibrary SHARED ${Justly_sources})

# need C++ 17 for qt
target_compile_features(JustlyLibrary PUBLIC cxx_std_17)

# apple bundles have a different data folder
if (APPLE)
    set(data_folder Resources)
else()
    set(data_folder share)
endif()

# where to look for the soundfont file
target_compile_definitions(
    JustlyLibrary PRIVATE
    "SOUNDFONT_RELATIVE_PATH=\"../${data_folder}/MuseScore_General.sf2\""
)

# private code
add_subdirectory(src)
# public headers
add_subdirectory(include)

# private include dir
target_include_directories(JustlyLibrary PRIVATE src)

# public include dir, separate for build and install interface
target_include_directories(JustlyLibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(JustlyLibrary PUBLIC
    nlohmann_json::nlohmann_json
    Qt6::Core
    Qt6::Widgets
    FluidSynth::libfluidsynth
)
target_link_libraries(JustlyLibrary PRIVATE nlohmann_json_schema_validator::validator)

# for dllexport instead of dllimport
target_compile_definitions(JustlyLibrary PRIVATE "JUSTLY_LIBRARY")

install(TARGETS JustlyLibrary
    RUNTIME_DEPENDENCY_SET RUNTIMES
    FILE_SET justly_public_headers
)

if (WIN32)
    install(RUNTIME_DEPENDENCY_SET RUNTIMES
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
        # folders to look in for platforms without an rpath
        DIRECTORIES
            # vcpkg libraries
            "${_VCPKG_INSTALLED_DIR}/{VCPKG_TARGET_TRIPLET}/lib"
    )
endif()

# not getting picked up as a runtime dependency?
install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)

function(qt_deploy executable_name)
    qt_generate_deploy_app_script(TARGET ${executable_name}
        OUTPUT_SCRIPT script_file
    )
    install(SCRIPT ${script_file})
endfunction()

# apple will deploy the library during the fixup bundle stage
if (WIN32)
    qt_deploy(JustlyLibrary)
endif()

function(install_executable executable_name)
    # install the executable, bundle into main folder
    install(TARGETS ${executable_name} BUNDLE DESTINATION ".")

    # deployment won't work on linux
    if (NOT LINUX)
        qt_deploy(${executable_name})
    endif()

    if (APPLE)
        # fixup the bundle
        set(INSTALLED_BUNDLE_FOLDER $<INSTALL_PREFIX>/${executable_name}.app)
        install(CODE "
            include(BundleUtilities)

            # find plugins
            file(GLOB INSTALLED_PLUGINS
                ${INSTALLED_BUNDLE_FOLDER}/Contents/PlugIns/*/*.dylib
            )
            # find framework folders
            file(GLOB INSTALLED_FRAMEWORK_FOLDERS
                ${INSTALLED_BUNDLE_FOLDER}/Contents/Frameworks/*.framework
            )
            # add regular libraries
            list(APPEND INSTALLED_FRAMEWORK_FOLDERS $<INSTALL_PREFIX>/lib)

            fixup_bundle(
                ${INSTALLED_BUNDLE_FOLDER}
                \"\${INSTALLED_PLUGINS}\"
                \"\${INSTALLED_FRAMEWORK_FOLDERS}\"
            )
        ")
    endif()
endfunction()

# add license
install(FILES ${Justly_SOURCE_DIR}/LICENSE.md DESTINATION .)
install(DIRECTORY ${Justly_SOURCE_DIR}/share/ DESTINATION ${data_folder})

qt_add_executable(Justly MACOSX_BUNDLE)
target_sources(Justly PRIVATE bin/main.cpp)
target_link_libraries(Justly PRIVATE JustlyLibrary Qt6::Widgets)
install_executable(Justly)

if (BUILD_TESTS)
    qt_add_executable(JustlyTests MACOSX_BUNDLE)
    add_subdirectory(tests)
    target_include_directories(JustlyTests PRIVATE ${Justly_SOURCE_DIR})
    target_link_libraries(JustlyTests PUBLIC JustlyLibrary)
    target_link_libraries(JustlyTests PRIVATE
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
        --config Release
    )
    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    # run the installed app
    # this is to test that we've installed everything we need
    if (APPLE)
        add_test(NAME run_tests COMMAND open ${test_install_folder}/JustlyTests.app) 
    else()
        add_test(NAME run_tests COMMAND ${test_install_folder}/bin/JustlyTests)
    endif()
    
    # run tests offscreen on linux
    if (LINUX)
        set_property(TEST run_tests PROPERTY ENVIRONMENT QT_QPA_PLATFORM=offscreen)
    endif()
endif()

