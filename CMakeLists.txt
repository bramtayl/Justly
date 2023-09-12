cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.9)

include(InstallRequiredSystemLibraries)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

option(JUSTLY_BUILD_TESTS "Build tests" ON)
option(JUSTLY_COMBINE_TESTS "Combine tests" ON)
option(JUSTLY_TRACK_COVERAGE "Track coverage" ON)

find_package(CSound CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

qt_standard_project_setup()

qt_add_library(justlyeditor 
    "src/Chord.cpp"
    "src/Chord.h"
    "src/ChordsModel.cpp"
    "src/ChordsModel.h"
    "src/ComboBoxDelegate.cpp"
    "src/ComboBoxDelegate.h"
    "src/commands.cpp"
    "src/commands.h"
    "src/Editor.cpp"
    "src/Editor.h"
    "src/Instrument.cpp"
    "src/Instrument.h"
    "src/InstrumentsModel.cpp"
    "src/InstrumentsModel.h"
    "src/Interval.h"
    "src/Interval.cpp"
    "src/IntervalDelegate.h"
    "src/IntervalDelegate.cpp"
    "src/IntervalEditor.h"
    "src/IntervalEditor.cpp"
    "src/JsonErrorHandler.h"
    "src/JsonErrorHandler.cpp"
    "src/Note.h"
    "src/Note.cpp"
    "src/NoteChord.h"
    "src/NoteChord.cpp"
    "src/Player.h"
    "src/Player.cpp"
    "src/ShowSlider.h"
    "src/ShowSlider.cpp"
    "src/Song.h"
    "src/Song.cpp"
    "src/ShowSliderDelegate.h"
    "src/ShowSliderDelegate.cpp"
    "src/SpinBoxDelegate.h"
    "src/SpinBoxDelegate.cpp"
    "src/SuffixedNumber.h"
    "src/SuffixedNumber.cpp"
    "src/StableIndex.h"
    "src/StableIndex.cpp"
    "src/Tester.h"
    "src/Tester.cpp"
    "src/TreeNode.h"
    "src/TreeNode.cpp"
    "src/utilities.h"
    "src/utilities.cpp"
)
if (LINUX)
    set_target_properties(justlyeditor PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
target_include_directories(justlyeditor PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)
target_include_directories(justlyeditor PRIVATE
    "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
)
target_link_libraries(justlyeditor PUBLIC 
    CSound::libcsnd6
    nlohmann_json_schema_validator
    Qt6::Widgets
    Qt6::Test
)
target_link_libraries(justlyeditor PRIVATE
    CSound::csound64
    Qt6::Test
)

qt_add_executable(Justly "src/main.cpp")
if (LINUX)
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()
target_link_libraries(Justly PRIVATE justlyeditor Qt6::Widgets)

if (JUSTLY_BUILD_TESTS)
    qt_add_executable(JustlyTests "src/test.cpp")
    if (LINUX)
        set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    endif()
    target_link_libraries(JustlyTests PRIVATE justlyeditor Qt6::Test)
    
    if (JUSTLY_TRACK_COVERAGE AND NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

    include(CTest)

    if (JUSTLY_COMBINE_TESTS)
        add_test(NAME test_justly COMMAND JustlyTests)
    else()
        add_test(NAME test_colors COMMAND JustlyTests test_colors)
        add_test(NAME test_column_headers COMMAND JustlyTests test_column_headers)
        add_test(NAME test_copy_paste COMMAND JustlyTests test_copy_paste)
        add_test(NAME test_delegates COMMAND JustlyTests test_delegates)
        add_test(NAME test_flags COMMAND JustlyTests test_flags)
        add_test(NAME test_get_value COMMAND JustlyTests test_get_value)
        add_test(NAME test_insert_delete COMMAND JustlyTests test_insert_delete)
        add_test(NAME test_json COMMAND JustlyTests test_json)
        add_test(NAME test_play COMMAND JustlyTests test_play)
        add_test(NAME test_select COMMAND JustlyTests test_select)
        add_test(NAME test_set_value COMMAND JustlyTests test_set_value)
        add_test(NAME test_controls COMMAND JustlyTests test_controls)
        add_test(NAME test_tree COMMAND JustlyTests test_tree)
        add_test(NAME test_view COMMAND JustlyTests test_view)
        add_test(NAME test_io COMMAND JustlyTests test_io)
    endif()
endif()

# TODO: bundles
if (NOT APPLE)
    install(TARGETS justlyeditor
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    install(TARGETS Justly
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    install(FILES "${Justly_SOURCE_DIR}/LICENSE.md"
        DESTINATION "."
    )

    install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
    install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General_License.md" TYPE DATA)
    install(FILES "${Justly_SOURCE_DIR}/share/instruments.csv" TYPE DATA)

    # this isn't getting picked up as a dependency so install manually
    # TODO: figure out why
    if (WIN32)
        install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
    else()
        install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE LIB)
    endif()

    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" DESTINATION TYPE BIN)

    if (LINUX)
        find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
        find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
            DESTINATION "plugins/platforms"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
            DESTINATION "plugins/platformthemes"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()

    if (WINDOWS)
        find_package(Qt6QWindowsIntegrationPlugin CONFIG REQUIRED)
        find_package(Qt6QWindowsVistaStyle CONFIG REQUIRED)
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QWindowsIntegrationPlugin
            DESTINATION "plugins/platforms"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QWindowsVistaStyle
            DESTINATION "plugins/styles"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()

    install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/csound/plugins64-6.0"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    install(RUNTIME_DEPENDENCY_SET Justly_linked
        PRE_EXCLUDE_REGEXES "."
        PRE_INCLUDE_REGEXES 
            "csnd"
            "csound"
            "FLAC"
            "icudata"
            "icui18n"
            "icuuc"
            "mp3lame"
            "mpg123"
            "ogg"
            "opus"
            "portaudio"
            "Qt6"
            "sndfile"
            "vorbis"
            "vorbisenc"
            "xcb-cursor"
        DIRECTORIES 
            "$<TARGET_FILE_DIR:CSound::csound64>"
            "$<TARGET_FILE_DIR:Qt6::Widgets>"
    )
endif()

include(CPack)

mark_as_advanced(Justly_linked)
