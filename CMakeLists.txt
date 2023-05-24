cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")

project(Justly VERSION 0.2.0)

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Csound REQUIRED)

set(Justly_DEPENDENCIES Qt6::Widgets ${CSOUND_LIBRARIES})

qt_standard_project_setup()

set(Justly_SOURCES
    "src/Chord.cpp"
    "src/CsoundData.cpp"
    "src/commands.cpp"
    "src/Editor.cpp"
    "src/JsonHelpers.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/Player.cpp"
    "src/Selector.cpp"
    "src/Song.cpp"
    "src/TreeNode.cpp"
)

add_executable(Justly ${Justly_SOURCES} "src/main.cpp")
target_link_libraries(Justly PUBLIC ${Justly_DEPENDENCIES})

add_executable(RunTests ${Justly_SOURCES} "src/Tester.cpp" "src/test.cpp")
target_compile_options(RunTests PRIVATE "--coverage" "-O0")
target_link_libraries(RunTests PUBLIC ${Justly_DEPENDENCIES})
target_link_libraries(RunTests PRIVATE  Qt6::Test "--coverage")

enable_testing()
add_test(NAME test COMMAND "${CMAKE_BINARY_DIR}/RunTests" "${Justly_SOURCE_DIR}/examples")




