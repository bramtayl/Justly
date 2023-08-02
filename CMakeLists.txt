cmake_minimum_required(VERSION 3.24)

option(BUILD_TESTS "Build tests" ON)
option(COMBINE_TESTS "Combine tests" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include what you use" ON)
option(TRACK_COVERAGE "Track coverage" ON)
option(QT_NO_CREATE_TARGETS "Don't create targets" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
endif()

project(Justly VERSION 0.2.0)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Test Widgets)
if (LINUX)
    find_package(Qt6 CONFIG REQUIRED COMPONENTS Gui)
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED HINTS "${Qt6Gui_DIR}")
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED HINTS "${Qt6Gui_DIR}")
endif()
find_package(SndFile CONFIG REQUIRED)
find_package(CSound CONFIG REQUIRED)

qt_standard_project_setup()

set(JUSTLY_SOURCES
    "src/Chord.cpp"
    "src/ChordsModel.cpp"
    "src/ComboBoxDelegate.cpp"
    "src/commands.cpp"
    "src/Editor.cpp"
    "src/Instrument.cpp"
    "src/InstrumentsModel.cpp"
    "src/Interval.cpp"
    "src/IntervalDelegate.cpp"
    "src/IntervalEditor.cpp"
    "src/utilities.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/Performer.cpp"
    "src/Player.cpp"
    "src/ShowSlider.cpp"
    "src/Song.cpp"
    "src/ShowSliderDelegate.cpp"
    "src/SpinBoxDelegate.cpp"
    "src/SuffixedNumber.cpp"
    "src/StableIndex.cpp"
    "src/TreeNode.cpp"
)

set(JUSTLY_INCLUDE_DIRS 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)

set(JUSTLY_DEPENDENCIES
    CSound::csound64 CSound::libcsnd6 Qt6::Widgets
)

qt_add_executable(Justly ${JUSTLY_SOURCES} "src/main.cpp")
target_link_libraries(Justly PUBLIC ${JUSTLY_DEPENDENCIES})
target_include_directories(Justly PUBLIC ${JUSTLY_INCLUDE_DIRS})

if (BUILD_TESTS)
    qt_add_executable(JustlyTests ${JUSTLY_SOURCES} "src/Tester.cpp" "src/test.cpp")
    target_link_libraries(JustlyTests PRIVATE ${JUSTLY_DEPENDENCIES})
    target_link_libraries(JustlyTests PUBLIC Qt6::Test)
    target_include_directories(JustlyTests PUBLIC ${JUSTLY_INCLUDE_DIRS})

    # add flags for coverage
    if (TRACK_COVERAGE AND NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

    include(CTest)

    if (COMBINE_TESTS)
        add_test(NAME JustlyTests COMMAND JustlyTests)
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
    install(TARGETS JustlyTests RUNTIME_DEPENDENCIES
        # this shouldn't be needed on Linux but it is???
        DIRECTORIES "$<TARGET_FILE_DIR:CSound::csound64>"
        # PRE_EXCLUDE_REGEXES ".*"
        PRE_INCLUDE_REGEXES    
            "libQt6Test"
    )
endif()

install(TARGETS Justly RUNTIME_DEPENDENCIES
    # this shouldn't be needed on Linux but it is???
    DIRECTORIES "$<TARGET_FILE_DIR:CSound::csound64>"
    # PRE_EXCLUDE_REGEXES ".*"
    PRE_INCLUDE_REGEXES "csound64" "csnd6"
)

install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "bin/csound_plugins"
    RUNTIME_DEPENDENCY_SET REAL_TIME_DEPENDENCIES
)
install(RUNTIME_DEPENDENCY_SET REAL_TIME_DEPENDENCIES
    # PRE_EXCLUDE_REGEXES ".*"
    PRE_INCLUDE_REGEXES "portaudio"
)

if(WIN32)
    # runtime dependencies are all system libraries
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.bat" TYPE BIN)
else()
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.sh" TYPE BIN)
endif()


if (LINUX)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET XCB_DEPENDENCIES
    )
    install(RUNTIME_DEPENDENCY_SET XCB_DEPENDENCIES
        # PRE_EXCLUDE_REGEXES ".*"
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "plugins/platformthemes"
        RUNTIME_DEPENDENCY_SET STYLE_DEPENDENCIES
    )
    install(RUNTIME_DEPENDENCY_SET THEME_DEPENDENCIES
        # PRE_EXCLUDE_REGEXES ".*"
    )
    install(FILES "${Justly_SOURCE_DIR}/etc/qt.conf"
        DESTINATION "bin"
    )
else()
    qt_generate_deploy_app_script(TARGET Justly
        FILENAME_VARIABLE deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

mark_as_advanced(JUSTLY_SOURCES JUSTLY_DEPENDENCIES JUSTLY_INCLUDE_DIRS)
