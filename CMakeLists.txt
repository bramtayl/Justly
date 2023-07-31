cmake_minimum_required(VERSION 3.24)

option(BUILD_TESTS ON)
option(COMBINE_TESTS OFF)
option(INCLUDE_WHAT_YOU_USE ON)
option(TRACK_COVERAGE ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
endif()

project(Justly VERSION 0.2.0)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Qt6Gui REQUIRED)

# to find CSound
find_package(CSound REQUIRED)

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

qt_add_executable(Justly ${JUSTLY_SOURCES} "src/main.cpp")
target_link_libraries(Justly PUBLIC CSound::csound64 CSound::libcsnd6 Qt6::Widgets)

if (BUILD_TESTS)
    qt_add_executable(JustlyTests ${JUSTLY_SOURCES} "src/Tester.cpp" "src/test.cpp")
    target_link_libraries(JustlyTests PUBLIC CSound::csound64 CSound::libcsnd6 Qt6::Widgets)
    target_link_libraries(JustlyTests PRIVATE Qt6::Test)

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

endif()

set(SYSTEM_LIBRARIES
    "^libasound\\."
    "^libc\\."
    "^ld-"
    "^libdb\\."
    "^libgcc_s\\."
    "^libGLX\\."
    "^libICE\\."
    "^libjack\\."
    "^libm\\."
    "^libmvec\\."
    "^libOpenGL\\."
    "^libpulse\\."
    "^libpulse-simple\\."
    "^libSM\\."
    "^libsndfile\."
    "^libstdc\\+\\+\\."
    "^libX11-xcb\\."
    "^libX11\\."
    "^libxcb\\."
    "^libxcb-cursor\\."
    "^libxcb-icccm\\."
    "^libxcb-image\\."
    "^libxcb-xinput\\."
    "^libxcb-keysyms\\."
    "^libxcb-randr\\."
    "^libxcb-render-util\\."
    "^libxcb-shm\\."
    "^libxcb-sync\\."
    "^libxcb-xfixes\\."
    "^libxcb-render\\."
    "^libxcb-shape\\."
    "^libxcb-xkb\\."
    "^libxkbcommon\\."
    "^libxkbcommon-x11\\."
    "^libXrender\\."
)

install(TARGETS Justly RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES ${SYSTEM_LIBRARIES}
    # this shouldn't be needed on Linux but it is???
    DIRECTORIES "$<TARGET_FILE_DIR:CSound::csound64>"
)

install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "bin/csound_plugins"
    RUNTIME_DEPENDENCY_SET REAL_TIME_DEPENDENCIES
)
install(RUNTIME_DEPENDENCY_SET REAL_TIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES ${SYSTEM_LIBRARIES}  
)

qt_generate_deploy_app_script(TARGET Justly OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

if(WIN32)
    # runtime dependencies are all system libraries
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.bat" TYPE BIN)
else()
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.sh" TYPE BIN)
endif()

if (BUILD_TESTS)
endif()

mark_as_advanced(JUSTLY_SOURCES JUSTLY_DEPENDENCIES SYSTEM_LIBRARIES)
