cmake_minimum_required(VERSION 3.25)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(JUSTLY_BUILD_TESTS "Build tests" ON)
option(JUSTLY_COMBINE_TESTS "Combine tests" ON)
option(JUSTLY_INCLUDE_WHAT_YOU_USE "Use include-what-you-use" OFF)
option(JUSTLY_TRACK_COVERAGE "Track coverage" ON)
option(JUSTLY_USE_CLANG "Use clang" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if (JUSTLY_INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
endif()

if (JUSTLY_USE_CLANG)
    set("CMAKE_C_COMPILER" "clang")
    set("CMAKE_CXX_COMPILER" "clang++")
endif()

project(Justly VERSION 0.3.3)

find_package(CSound CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Test Widgets)
find_package(SndFile CONFIG REQUIRED)

# get_property(imported_targets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
# message("${imported_targets}")

qt_standard_project_setup()

set(Justly_sources
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
    "src/Interval.cpp"
    "src/Interval.h"
    "src/IntervalDelegate.cpp"
    "src/IntervalDelegate.h"
    "src/IntervalEditor.cpp"
    "src/IntervalEditor.h"
    "src/Note.cpp"
    "src/Note.h"
    "src/NoteChord.cpp"
    "src/NoteChord.h"
    "src/Performer.cpp"
    "src/Performer.h"
    "src/Player.cpp"
    "src/Player.h"
    "src/ShowSlider.cpp"
    "src/ShowSlider.h"
    "src/Song.cpp"
    "src/Song.h"
    "src/ShowSliderDelegate.cpp"
    "src/ShowSliderDelegate.h"
    "src/SpinBoxDelegate.cpp"
    "src/SpinBoxDelegate.h"
    "src/SuffixedNumber.cpp"
    "src/SuffixedNumber.h"
    "src/StableIndex.cpp"
    "src/StableIndex.h"
    "src/TreeNode.cpp"
    "src/TreeNode.h"
    "src/utilities.cpp"
    "src/utilities.h"
)

set(Justly_include_folders 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

set(Justly_dependencies
    CSound::csound64
    CSound::libcsnd6
    Qt6::Widgets
)

qt_add_executable(Justly ${Justly_sources} "src/main.cpp")
set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
target_include_directories(Justly PUBLIC ${Justly_include_folders})
target_link_libraries(Justly PUBLIC ${Justly_dependencies})

if (APPLE)
    set_target_properties(Justly PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

install(TARGETS Justly
    RUNTIME_DEPENDENCY_SET Justly_linked
    BUNDLE DESTINATION .
)
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md"
    DESTINATION "."
)
install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "plugins"
    RUNTIME_DEPENDENCY_SET Justly_linked
)

if (LINUX)
    # deploy manually, GRRR
        
    find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)

    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED HINTS "${Qt6Gui_DIR}")
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED HINTS "${Qt6Gui_DIR}")

    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "plugins/platformthemes"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    find_package(XCB REQUIRED COMPONENTS CURSOR)
    install(FILES "${XCB_CURSOR_LIBRARY}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )

    install(FILES "${Justly_SOURCE_DIR}/etc/qt.conf"
        DESTINATION "bin"
    )
else()
    if (Qt6_VERSION VERSION_LESS "6.5")
        qt_generate_deploy_app_script(TARGET Justly
            FILENAME_VARIABLE deploy_script
        )
    else()
        qt_generate_deploy_app_script(TARGET Justly
            OUTPUT_SCRIPT deploy_script
        )
    endif()
    install(SCRIPT ${deploy_script})
endif()

if (JUSTLY_BUILD_TESTS)
    qt_add_executable(JustlyTests ${Justly_sources} "src/Tester.cpp" "src/test.cpp")
    target_include_directories(JustlyTests PUBLIC ${Justly_include_folders})
    set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    target_link_libraries(JustlyTests PUBLIC ${Justly_dependencies})
    target_link_libraries(JustlyTests PRIVATE Qt6::Test)
    
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

set(Justly_runtime_regexes 
    "csnd"
    "csound"
    "portaudio"
    "Qt6"
    "icui18n"
    "icuuc"
    "icudata"
)

if (WIN32)
    # no rpath so specify directories
    install(RUNTIME_DEPENDENCY_SET Justly_linked
        PRE_EXCLUDE_REGEXES "."
        PRE_INCLUDE_REGEXES ${Justly_runtime_regexes}
        DIRECTORIES 
            "$<TARGET_FILE_DIR:CSound::csound64>"
            "$<TARGET_FILE_DIR:Qt6::Widgets>"
    )
else()
    install(RUNTIME_DEPENDENCY_SET Justly_linked
        PRE_EXCLUDE_REGEXES "."
        PRE_INCLUDE_REGEXES ${Justly_runtime_regexes}
    )
endif()

include(CPack)

if (WIN32)
    set(CPACK_SOURCE_GENERATOR "ZIP")
else()
    set(CPACK_SOURCE_GENERATOR "TGZ") 
endif()

mark_as_advanced(
    Justly_sources
    Justly_dependencies
    Justly_include_folders
    Justly_linked
    Justly_runtime_regexes
)
