cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11 LANGUAGES CXX)

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CPack)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" ON)
option(TRACK_COVERAGE "Track coverage" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(CLANG_TIDY "Run clang-tidy" OFF)
option(CPPLINT "Run cpplint" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# only will work on my windows boot
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)
find_package(FluidSynth CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator CONFIG REQUIRED)
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)

qt_standard_project_setup()

check_compiler_flag(CXX -g has_debug_info)

find_program(clang_tidy_executable NAMES clang-tidy)
find_program(include_what_you_use_executable NAMES include-what-you-use)

if (INCLUDE_WHAT_YOU_USE AND NOT (include_what_you_use_executable STREQUAL "include_what_you_use_executable-NOTFOUND"))
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        ${include_what_you_use_executable}
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
    )
endif()

if (CLANG_TIDY AND NOT (clang_tidy_executable STREQUAL "clang_tidy_executable-NOTFOUND"))
    set(CMAKE_CXX_CLANG_TIDY ${clang_tidy_executable})
endif()

check_compiler_flag(CXX -Wall has_all_warnings)
if (has_all_warnings)
    add_compile_options(-Wall)
endif()

check_compiler_flag(CXX -Wextra has_extra_warnings)
if (has_extra_warnings)
    add_compile_options(-Wextra)
endif()

if (TRACK_COVERAGE)
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

qt_add_library(JustlyLibrary ${Justly_sources})

add_subdirectory(src)
add_subdirectory(include)

set(plugins_folder plugins)
set(data_folder share)

target_compile_definitions(
    JustlyLibrary PRIVATE
    "SOUNDFONT_RELATIVE_PATH=\"../${data_folder}/MuseScore_General.sf2\""
)

target_include_directories(JustlyLibrary PRIVATE ${Justly_SOURCE_DIR})

target_include_directories(JustlyLibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)

target_link_libraries(JustlyLibrary PUBLIC
    nlohmann_json::nlohmann_json
    Qt6::Core
    Qt6::Widgets
)

target_link_libraries(JustlyLibrary PRIVATE
    nlohmann_json_schema_validator
)

install(TARGETS JustlyLibrary
    RUNTIME_DEPENDENCY_SET RUNTIMES
    FILE_SET justly_public_headers
    COMPONENT Library
)

if (LINUX)
    # the plugins are submodules of Qt6::Gui
    list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES ${Justly_SOURCE_DIR}/bin/qt.conf TYPE BIN)

    set_property(TARGET ${target_name} PROPERTY INSTALL_RPATH $ORIGIN/../lib)

    # to pick up the xcb-cursor runtime dependency
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION ${plugins_folder}/platforms
        RUNTIME_DEPENDENCY_SET RUNTIMES
        COMPONENT Library
    )

    # to look nice
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION ${plugins_folder}/platformthemes
        RUNTIME_DEPENDENCY_SET RUNTIMES
        COMPONENT Library
    )

    # this allows tests to run without a physical screen
    find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
        DESTINATION ${plugins_folder}/platforms
        RUNTIME_DEPENDENCY_SET RUNTIMES
        COMPONENT Library
    )
endif()

qt_generate_deploy_app_script(TARGET JustlyLibrary
    FILENAME_VARIABLE script_file
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${script_file} COMPONENT Library)

install(RUNTIME_DEPENDENCY_SET RUNTIMES
    PRE_INCLUDE_REGEXES
        # on linux, Qt needs an old ICU version
        Qt6Core
            icudata
            icui18n
            icuuc
            Qt6Gui
                Qt6DBus
            Qt6Widgets
            Qt6XcbQpa
                # not shipped by default on Ubuntu so add it
                xcb-cursor
    PRE_EXCLUDE_REGEXES .
    # folders to look in for platforms without an rpath
    DIRECTORIES 
        # vcpkg libraries
        $<TARGET_FILE_DIR:nlohmann_json_schema_validator>
        # qt libraries
        $<TARGET_FILE_DIR:Qt6::Widgets>
    COMPONENT Library
)

# add license
install(FILES ${Justly_SOURCE_DIR}/LICENSE.md DESTINATION .)
install(DIRECTORY ${Justly_SOURCE_DIR}/share/ DESTINATION ${data_folder})

qt_add_executable(Justly)
target_sources(Justly PRIVATE src/main.cpp)
target_link_libraries(Justly PRIVATE JustlyLibrary Qt6::Widgets FluidSynth::libfluidsynth)
install(TARGETS Justly COMPONENT Executable)

if (BUILD_TESTS)
    qt_add_executable(JustlyTests)
    add_subdirectory(tests)
    target_include_directories(JustlyTests PRIVATE ${Justly_SOURCE_DIR})
    target_link_libraries(JustlyTests PUBLIC JustlyLibrary)
    target_link_libraries(JustlyTests PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Widgets
        FluidSynth::libfluidsynth
    )
    set_property(TARGET JustlyTests PROPERTY INSTALL_RPATH "$ORIGIN/../lib")
    install(TARGETS JustlyTests COMPONENT Tests)

    if (LINUX)
        set(platform_assignment QT_QPA_PLATFORM=offscreen)
    else()
        set(platform_assignment "")
    endif()

    set(test_install_folder ${Justly_SOURCE_DIR}/test_install)

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR} --prefix ${test_install_folder}
    )
    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    enable_testing(true)
    add_test(NAME run_tests COMMAND ${test_install_folder}/bin/JustlyTests)
    set_property(TEST run_tests PROPERTY ENVIRONMENT ${platform_assignment})
endif()
