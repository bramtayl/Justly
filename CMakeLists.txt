cmake_minimum_required(VERSION 3.24)
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
enable_testing(true)

# when you change this, change the version numbers in .github/workflows/upload_binaries.yaml too
project(Justly VERSION 0.1.0)

# CPack will use the first configuration
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_BUILD_TYPE}")

list(APPEND CMAKE_MODULE_PATH "${Justly_SOURCE_DIR}/cmake")
# looks in Qt6_ROOT from the environment too
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets Test)
find_package(Gamma REQUIRED)

set(Justly_SOURCES
    "src/Chord.cpp"
    "src/commands.cpp"
    "src/DefaultInstrument.cpp"
    "src/Editor.cpp"
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
    "src/TestJustly.cpp"
    "src/test.cpp"
)

# allows for qt macros
set_property(TARGET Justly PROPERTY AUTOMOC ON)
set_property(TARGET RunTests PROPERTY AUTOMOC ON)

target_link_libraries(Justly PUBLIC Qt6::Widgets Gamma::gamma)
target_link_libraries(RunTests PUBLIC Qt6::Widgets Gamma::gamma)
target_link_libraries(RunTests PRIVATE Qt6::Test)

# coverage not available on MSVC
if (NOT MSVC)
    # no optimization for more accurate coverage
    target_compile_options(RunTests PRIVATE "--coverage" "-O0")
    target_link_libraries(RunTests PRIVATE "--coverage")
endif()

install(TARGETS Justly RunTests
    RUNTIME_DEPENDENCIES
        # exclude some junk dependencies
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        # make sure qt runtime dependencies are found
        DIRECTORIES "$<TARGET_FILE_DIR:Qt6::Widgets>"
    # needed for macOS I guess
    FRAMEWORK DESTINATION "bin"
)

# on other platforms, the plugins get statically linked
if (WIN32)
    # from PREFIX/include, go back to PREFIX, then into plugins
    set(PLUGINS_FOLDER "$<TARGET_FILE_DIR:Qt6::Widgets>/../plugins")

    # debug libraries end with d
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    else()
        set(DEBUG_SUFFIX "")
    endif()

    install(FILES "${PLUGINS_FOLDER}/platforms/qwindows${DEBUG_SUFFIX}.dll" DESTINATION "bin/platforms")
    install(FILES "${PLUGINS_FOLDER}/platforms/qwindows${DEBUG_SUFFIX}.pdb" DESTINATION "bin/platforms")

    # for testing
    install(FILES "${PLUGINS_FOLDER}/platforms/qoffscreen${DEBUG_SUFFIX}.dll" DESTINATION "bin/platforms")
    install(FILES "${PLUGINS_FOLDER}/platforms/qoffscreen${DEBUG_SUFFIX}.pdb" DESTINATION "bin/platforms")

    # to look nice
    install(FILES "${PLUGINS_FOLDER}/styles/qwindowsvistastyle${DEBUG_SUFFIX}.dll" DESTINATION "bin/styles")
    install(FILES "${PLUGINS_FOLDER}/styles/qwindowsvistastyle${DEBUG_SUFFIX}.pdb" DESTINATION "bin/styles")
endif()

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

include(CPack)