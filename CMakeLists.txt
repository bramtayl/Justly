cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11)

option(BUILD_EXECUTABLE "Build executable" ON)
option(BUILD_TESTS "Build tests" ON)
option(WARNINGS "Print warnings" ON)
option(COVERAGE "Track coverage" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" ON)
option(CLANG_TIDY "Run clang-tidy" OFF)
option(CPPLINT "Run cpplint" ON)

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CPack)
include(InstallRequiredSystemLibraries)

# helpful for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# only will work on my windows boot
# list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

find_package(CSound CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    find_program(include_what_you_use NAMES "include-what-you-use" REQUIRED)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        "${include_what_you_use}"
        "-Xiwyu"
        "--mapping_file=${Justly_SOURCE_DIR}/map.imp"
    )
endif()

if (CLANG_TIDY)
    find_program(clang_tidy NAMES "clang-tidy" REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${clang_tidy}")
endif()

if (CPPLINT)
    find_program(cpp_lint NAMES "cpplint" REQUIRED)
    set(CMAKE_CXX_CPPLINT "${cpp_lint}"
        "--quiet"
        "--exclude=**/mocs_compilation.cpp"
    )
endif()

check_compiler_flag("CXX" "-Wall" has_all_warnings)
check_compiler_flag("CXX" "-Wextra" has_extra_warnings)
if (WARNINGS AND has_all_warnings AND has_extra_warnings)
    add_compile_options("-Wall")
    add_compile_options("-Wextra")
endif()

check_compiler_flag("CXX" "-g" has_debug_info)
if (CMAKE_BUILD_TYPE STREQUAL Debug AND has_debug_info)
    add_compile_options("-g")
endif()

check_compiler_flag("CXX" "-Og" has_debug_optimization)
check_linker_flag("CXX" "--coverage" has_link_coverage)

if (COVERAGE AND has_debug_optimization AND has_link_coverage)
    add_compile_options("-Og" "--coverage")
    add_link_options("--coverage")
endif()

qt_add_library(JustlyLibrary ${Justly_sources})

set(TARGETS "JustlyLibrary")

target_sources(JustlyLibrary PRIVATE
    "src/editors/InstrumentEditor.cpp"
    "src/editors/IntervalEditor.cpp"
    "src/editors/IntervalEditor.h"

    "src/main/Editor.cpp"
    "src/main/MyDelegate.h"

    "src/main/MyDelegate.cpp"
    "src/main/MyView.h"
    "src/main/MyView.cpp"
    "src/main/Player.cpp"
    "src/main/Song.cpp"

    "src/metatypes/Instrument.cpp"
    "src/metatypes/Interval.cpp"

    "src/models/ChordsModel.cpp"
    "src/models/InstrumentsModel.h"
    "src/models/InstrumentsModel.cpp"

    "src/notechord/Chord.cpp"
    "src/notechord/Note.cpp"
    "src/notechord/NoteChord.cpp"

    "src/commands/CellChange.h"
    "src/commands/CellChange.cpp"
    "src/commands/InsertRemoveChange.h"
    "src/commands/InsertRemoveChange.cpp"
    "src/commands/InsertEmptyChange.h"
    "src/commands/InsertEmptyChange.cpp"
    "src/commands/StartingValueChange.h"
    "src/commands/StartingValueChange.cpp"

    "src/utilities/JsonErrorHandler.h"
    "src/utilities/JsonErrorHandler.cpp"
    "src/test/Tester.cpp"
)

target_sources(JustlyLibrary PUBLIC
    FILE_SET justly_headers
    TYPE HEADERS
    BASE_DIRS "include"
    FILES
    "include/editors/InstrumentEditor.h"

    "include/main/Editor.h"
    "include/main/Player.h"
    "include/main/Song.h"

    "include/metatypes/Instrument.h"
    "include/metatypes/Interval.h"

    "include/models/ChordsModel.h"

    "include/notechord/Note.h"
    "include/notechord/NoteChord.h"
    "include/notechord/Chord.h"

    "include/test/Tester.h"

    "include/utilities/SongIndex.h"
)

set(plugins_folder "plugins")
set(data_folder "share")

target_compile_definitions(
    JustlyLibrary PRIVATE
    "PLUGINS_RELATIVE_PATH=\"../${plugins_folder}/csound\""
    "SOUNDFONT_RELATIVE_PATH=\"../${data_folder}/MuseScore_General.sf2\""
    "INSTRUMENTS_RELATIVE_PATH=\"../${data_folder}/instruments.csv\""
    "REALTIME_PROVIDER=\"pa\""
)

target_include_directories(JustlyLibrary PRIVATE
    "${Justly_SOURCE_DIR}/src"
    ${FAST_CPP_CSV_PARSER_INCLUDE_DIR}
)

target_include_directories(JustlyLibrary PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)

target_link_libraries(JustlyLibrary PUBLIC
    CSound::libcsnd6
    Microsoft.GSL::GSL
    nlohmann_json_schema_validator
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_link_libraries(JustlyLibrary PRIVATE
    CSound::csound64
    Qt6::Test
)

install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "${plugins_folder}/csound"
    RUNTIME_DEPENDENCY_SET RUNTIMES
)

install(TARGETS JustlyLibrary
    RUNTIME_DEPENDENCY_SET RUNTIMES
    FILE_SET justly_headers
)

if (LINUX)
    # the plugins are submodules of Qt6::Gui
    list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" TYPE BIN)

    set_property(TARGET ${target_name} PROPERTY INSTALL_RPATH "$ORIGIN/../lib")

    # to pick up the xcb-cursor runtime dependency
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "${plugins_folder}/platforms"
        RUNTIME_DEPENDENCY_SET RUNTIMES
    )

    # to look nice
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "${plugins_folder}/platformthemes"
        RUNTIME_DEPENDENCY_SET RUNTIMES
    )

    # this allows tests to run without a physical screen
    find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
        DESTINATION "${plugins_folder}/platforms"
        RUNTIME_DEPENDENCY_SET RUNTIMES
    )
elseif(WIN32)
    # some runtime dependencies aren't picked up by windows
    # TODO: figure out why?
    find_package(FLAC CONFIG REQUIRED)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
endif()

qt_generate_deploy_app_script(TARGET JustlyLibrary
    FILENAME_VARIABLE "SCRIPT_FILE"
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT "${SCRIPT_FILE}")

install(RUNTIME_DEPENDENCY_SET RUNTIMES
    PRE_INCLUDE_REGEXES
        "csnd"
        "csound"
        "FLAC"
        # on linux, Qt needs an old ICU version
        "icudata"
        "icui18n"
        "icuuc"
        "mp3lame"
        "mpg123"
        "nlohmann_json_schema_validator"
        "ogg"
        "opus"
        "portaudio"
        "Qt6"
        "sndfile"
        "vorbis"
        "vorbisenc"
        # not shipped by default on Ubuntu so add it
        "xcb-cursor"
    PRE_EXCLUDE_REGEXES "."
    # folders to look in for platforms without an rpath
    DIRECTORIES 
        # vcpkg libraries
        "$<TARGET_FILE_DIR:CSound::csound64>"
        # qt libraries
        "$<TARGET_FILE_DIR:Qt6::Widgets>"
)

# add license
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md" DESTINATION ".")
install(DIRECTORY "${Justly_SOURCE_DIR}/share/" DESTINATION ${data_folder})

if (BUILD_EXECUTABLE)
    qt_add_executable(Justly)

    target_sources(
        Justly PUBLIC "src/executables/main.cpp"
    )

    target_link_libraries(Justly PRIVATE
        JustlyLibrary
        Qt6::Widgets
    )

    install(TARGETS Justly)
endif()

if (BUILD_TESTS)
    qt_add_executable(JustlyTests)

    if (COVERAGE AND has_debug_optimization AND has_link_coverage)
        target_compile_options(JustlyTests PRIVATE "-Og" "--coverage")
        target_link_options(JustlyTests PRIVATE "--coverage")
    endif()

    target_sources(
        JustlyTests PRIVATE "src/executables/test.cpp"
    )

    target_link_libraries(JustlyTests PRIVATE
        JustlyLibrary
        Qt6::Test
    )

    install(TARGETS JustlyTests)
endif()

