cmake_minimum_required(VERSION 3.24)

option(JUSTLY_BUILD_TESTS "Build tests" ON)
option(JUSTLY_COMBINE_TESTS "Combine tests" OFF)
option(JUSTLY_INCLUDE_WHAT_YOU_USE "Use include-what-you-use" OFF)
option(JUSTLY_TRACK_COVERAGE "Track coverage" ON)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

if (JUSTLY_INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
endif()

project(Justly VERSION 0.2.0)

find_package(CSound CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Test Widgets)
find_package(SndFile CONFIG REQUIRED)

qt_standard_project_setup()

set(Justly_sources
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

install(TARGETS Justly RUNTIME_DEPENDENCY_SET Justly_linked)
install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "plugins"
    RUNTIME_DEPENDENCY_SET Justly_linked
)

if(WIN32)
    # runtime dependencies are all system libraries
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.bat" TYPE BIN)
else()
    install(PROGRAMS "${Justly_SOURCE_DIR}/etc/Justly.ps1" TYPE BIN)
endif()

if (LINUX AND Qt6_VERSION VERSION_LESS "6.5")
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

    install(FILES "${Justly_SOURCE_DIR}/etc/qt.conf"
        DESTINATION "bin"
    )
else()
    qt_generate_deploy_app_script(TARGET Justly
        FILENAME_VARIABLE deploy_script
    )
    install(SCRIPT ${deploy_script})
endif()

if (JUSTLY_BUILD_TESTS)
    qt_add_executable(JustlyTests ${Justly_sources} "src/Tester.cpp" "src/test.cpp")
    target_include_directories(JustlyTests PUBLIC ${Justly_include_folders})
    set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    target_link_libraries(JustlyTests PRIVATE ${Justly_dependencies})
    target_link_libraries(JustlyTests PUBLIC Qt6::Test)
    
    if (JUSTLY_TRACK_COVERAGE AND NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

    include(CTest)

    if (JUSTLY_COMBINE_TESTS)
        add_test(name JustlyTests command JustlyTests)
    else()
        add_test(name test_colors command JustlyTests test_colors)
        add_test(name test_column_headers command JustlyTests test_column_headers)
        add_test(name test_copy_paste command JustlyTests test_copy_paste)
        add_test(name test_delegates command JustlyTests test_delegates)
        add_test(name test_flags command JustlyTests test_flags)
        add_test(name test_get_value command JustlyTests test_get_value)
        add_test(name test_insert_delete command JustlyTests test_insert_delete)
        add_test(name test_json command JustlyTests test_json)
        add_test(name test_play command JustlyTests test_play)
        add_test(name test_select command JustlyTests test_select)
        add_test(name test_set_value command JustlyTests test_set_value)
        add_test(name test_controls command JustlyTests test_controls)
        add_test(name test_tree command JustlyTests test_tree)
        add_test(name test_view command JustlyTests test_view)
        add_test(name test_io command JustlyTests test_io)
    endif()
endif()

install(RUNTIME_DEPENDENCY_SET Justly_linked
    PRE_EXCLUDE_REGEXES "."
    PRE_INCLUDE_REGEXES
        "csnd"
        "csound"
        "portaudio"
        "Qt6"
)

include(CPack)

mark_as_advanced(
    Justly_sources
    Justly_dependencies
    Justly_include_folders
    Justly_linked
)
