cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")

project(Justly VERSION 0.2.0)

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(CSound REQUIRED)

qt_standard_project_setup()

set(Justly_sources
    "src/Chord.cpp"
    "src/ComboBoxItemDelegate.cpp"
    "src/commands.cpp"
    "src/Editor.cpp"
    "src/Utilities.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/ShowSlider.cpp"
    "src/Song.cpp"
    "src/SliderItemDelegate.cpp"
    "src/SpinBoxItemDelegate.cpp"
    "src/TreeNode.cpp"
)

qt_add_executable(Justly ${Justly_sources} "src/main.cpp")
target_link_libraries(Justly PUBLIC Qt6::Widgets ${CSOUND_LIBRARIES})

qt_add_executable(JustlyTests ${Justly_sources} "src/Tester.cpp" "src/test.cpp")
target_compile_options(JustlyTests PRIVATE "--coverage" "-O0")
target_link_libraries(JustlyTests PUBLIC Qt6::Widgets Qt6::Test ${CSOUND_LIBRARIES})
target_link_libraries(JustlyTests PRIVATE "--coverage")

enable_testing(true)
add_test(NAME JustlyTests COMMAND JustlyTests)
add_test(NAME test_insert_delete COMMAND JustlyTests test_insert_delete)
add_test(NAME test_json COMMAND JustlyTests test_json)

mark_as_advanced(Justly_sources)

