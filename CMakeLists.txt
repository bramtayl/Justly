cmake_minimum_required(VERSION 3.24)

include(InstallRequiredSystemLibraries)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
SET(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

set(COMBINE_TESTS TRUE)
mark_as_advanced(COMBINE_TESTS)

project(Justly VERSION 0.2.0)

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(CSound REQUIRED)

qt_standard_project_setup()

set(JUSTLY_SOURCES
    "src/Chord.cpp"
    "src/ChordsModel.cpp"
    "src/ComboBoxDelegate.cpp"
    "src/commands.cpp"
    "src/Editor.cpp"
    "src/InstrumentsModel.cpp"
    "src/Interval.cpp"
    "src/IntervalDelegate.cpp"
    "src/IntervalEditor.cpp"
    "src/utilities.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/Instrument.cpp"
    "src/ShowSlider.cpp"
    "src/Song.cpp"
    "src/ShowSliderDelegate.cpp"
    "src/SpinBoxDelegate.cpp"
    "src/SuffixedNumber.cpp"
    "src/StableIndex.cpp"
    "src/TreeNode.cpp"
)

qt_add_executable(Justly ${JUSTLY_SOURCES} "src/main.cpp")
target_link_libraries(Justly PUBLIC Qt6::Widgets CSound::csound64 CSound::csnd6)

qt_add_executable(JustlyTests ${JUSTLY_SOURCES} "src/Tester.cpp" "src/test.cpp")
target_link_libraries(JustlyTests PUBLIC Qt6::Widgets Qt6::Test CSound::csound64 CSound::csnd6)

if (NOT MSVC)
    target_compile_options(JustlyTests PRIVATE "--coverage" "-O0")
    target_link_libraries(JustlyTests PRIVATE "--coverage")
endif()

include(CTest)
enable_testing(true)

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
    add_test(NAME test_save COMMAND JustlyTests test_save)
    add_test(NAME test_select COMMAND JustlyTests test_select)
    add_test(NAME test_set_value COMMAND JustlyTests test_set_value)
    add_test(NAME test_controls COMMAND JustlyTests test_controls)
    add_test(NAME test_tree COMMAND JustlyTests test_tree)
    add_test(NAME test_view COMMAND JustlyTests test_view)
endif()

mark_as_advanced(JUSTLY_SOURCES)

install(TARGETS Justly
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS Justly RUNTIME_DEPENDENCIES
  # exclude some junk dependencies
  PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
  POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
  DIRECTORIES "$<TARGET_FILE_DIR:CSound::csound64>" "$<TARGET_FILE_DIR:Qt6::Widgets>" "$<TARGET_FILE_DIR:CSound::csnd6>"
)
install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(FILES "${CSound_portaudio_DLL}" TYPE BIN)

# Generate the deployment script
qt_generate_deploy_app_script(
    TARGET Justly
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

# Call the deployment script
install(SCRIPT ${deploy_script})
