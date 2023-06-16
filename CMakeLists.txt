cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")

project(Justly VERSION 0.2.0)

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Csound REQUIRED)

qt_standard_project_setup()

set(Justly_SOURCES
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

qt_add_executable(Justly ${Justly_SOURCES} "src/main.cpp")
target_link_libraries(Justly PUBLIC Qt6::Widgets ${CSOUND_LIBRARIES})

qt_add_executable(RunTests ${Justly_SOURCES} "src/Tester.cpp" "src/test.cpp")
target_compile_options(RunTests PRIVATE "--coverage" "-O0")
target_link_libraries(RunTests PUBLIC Qt6::Widgets Qt6::Test ${CSOUND_LIBRARIES})
target_link_libraries(RunTests PRIVATE "--coverage")

enable_testing(true)
add_test(NAME test_orchestra COMMAND RunTests test_orchestra)
add_test(NAME test_column_headers COMMAND RunTests test_column_headers)
add_test(NAME test_insert_delete COMMAND RunTests test_insert_delete)
add_test(NAME test_colors COMMAND RunTests test_colors)
add_test(NAME test_get_value COMMAND RunTests test_get_value)
add_test(NAME test_set_value COMMAND RunTests test_set_value)
add_test(NAME test_play COMMAND RunTests test_play)
add_test(NAME test_flags COMMAND RunTests test_flags)
add_test(NAME test_tree COMMAND RunTests test_tree)
add_test(NAME test_save COMMAND RunTests test_save)
add_test(NAME test_select COMMAND RunTests test_select)
add_test(NAME test_json COMMAND RunTests test_json)
add_test(NAME test_view COMMAND RunTests test_view)
add_test(NAME test_sliders COMMAND RunTests test_sliders)

