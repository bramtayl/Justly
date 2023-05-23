cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
enable_testing(true)

# when you change this, change the version numbers in .github/workflows/upload_binaries.yaml too
project(Justly VERSION 0.1.0)

set(TRACK_COVERAGE FALSE)

# CPack will use the first configuration
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")
# looks in Qt6_ROOT from the environment too
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
find_package(Csound REQUIRED)

qt_standard_project_setup()

set(Justly_SOURCES
    "src/Chord.cpp"
    "src/CsoundData.cpp"
    "src/commands.cpp"
    "src/Editor.cpp"
    "src/Selector.cpp"
    "src/TreeNode.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/Player.cpp"
    "src/Song.cpp"
)

add_executable(Justly
    ${Justly_SOURCES}
    "src/main.cpp"
)
add_executable(RunTests
    ${Justly_SOURCES}
    "src/Tester.cpp"
    "src/test.cpp"
)

# allows for qt macros
set_property(TARGET Justly PROPERTY MACOSX_BUNDLE ON)
set_property(TARGET RunTests PROPERTY MACOSX_BUNDLE ON)

target_link_libraries(Justly PUBLIC Qt6::Widgets ${CSOUND_LIBRARIES})
target_link_libraries(RunTests PUBLIC Qt6::Widgets Qt6::Test ${CSOUND_LIBRARIES})

install(TARGETS Justly
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(TARGETS RunTests
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# coverage not available on MSVC
if (TRACK_COVERAGE AND (NOT MSVC))
    # no optimization for more accurate coverage
    target_compile_options(RunTests PRIVATE "--coverage" "-O0")
    target_link_libraries(RunTests PRIVATE "--coverage")
endif()

include(InstallRequiredSystemLibraries)
include(CPack)
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")

# call installed binary
# point to examples folder
add_test(
    NAME RunTests
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/RunTests" "${Justly_SOURCE_DIR}/examples"
)

set_property(
    TEST RunTests
    # offscreen so can run on CI
    PROPERTY ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

install(TARGETS Justly DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_app_script(TARGET Justly FILENAME_VARIABLE justly_deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${justly_deploy_script})

qt_generate_deploy_app_script(TARGET RunTests FILENAME_VARIABLE runtests_deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${runtests_deploy_script})
