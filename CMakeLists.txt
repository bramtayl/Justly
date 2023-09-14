cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.9)

include(InstallRequiredSystemLibraries)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

option(JUSTLY_BUILD_TESTS "Build tests" ON)
option(JUSTLY_COMBINE_TESTS "Combine tests" ON)
option(JUSTLY_TRACK_COVERAGE "Track coverage" ON)

find_package(CSound CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")


list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})



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
    "src/JsonErrorHandler.cpp"
    "src/Note.cpp"
    "src/NoteChord.cpp"
    "src/Player.cpp"
    "src/ShowSlider.cpp"
    "src/Song.cpp"
    "src/ShowSliderDelegate.cpp"
    "src/SpinBoxDelegate.cpp"
    "src/SuffixedNumber.cpp"
    "src/StableIndex.cpp"
    "src/TreeNode.cpp"
    "src/utilities.cpp"
)

set(Justly_include_folders 
    "${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
)

set(Justly_dependencies
    CSound::csound64
    CSound::libcsnd6
    Qt6::Widgets
    nlohmann_json_schema_validator
)

qt_add_executable(Justly ${Justly_sources} "src/main.cpp")
if (LINUX)
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()
target_include_directories(Justly PRIVATE ${Justly_include_folders})
target_link_libraries(Justly PUBLIC ${Justly_dependencies})

if (JUSTLY_BUILD_TESTS)
    qt_add_executable(JustlyTests ${Justly_sources}
        "src/Tester.h"
        "src/Tester.cpp"
        "src/test.cpp"
    )
    target_include_directories(JustlyTests PRIVATE ${Justly_include_folders})
    if (LINUX)
        set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    endif()
    target_link_libraries(JustlyTests PUBLIC ${Justly_dependencies})
    target_link_libraries(JustlyTests PRIVATE Qt6::Test)
    
    if (JUSTLY_TRACK_COVERAGE AND NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

    install(TARGETS JustlyTests
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
endif()

install(TARGETS Justly
    RUNTIME_DEPENDENCY_SET Justly_linked
)

install(FILES "${Justly_SOURCE_DIR}/LICENSE.md"
    DESTINATION "."
)

install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General_License.md" TYPE DATA)
install(FILES "${Justly_SOURCE_DIR}/share/instruments.csv" TYPE DATA)

# this isn't getting picked up as a dependency so install manually
# TODO: figure out why
if (WIN32)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
endif()

install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" DESTINATION TYPE BIN)

if (APPLE)
    find_package(Qt6QCocoaIntegrationPlugin CONFIG REQUIRED)
    find_package(Qt6QMacStyle CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QCocoaIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QMacStyle
        DESTINATION "plugins/styles"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
endif()

if (LINUX)
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "plugins/platformthemes"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
endif()

if (WINDOWS)
    find_package(Qt6QWindowsIntegrationPlugin CONFIG REQUIRED)
    find_package(Qt6QWindowsVistaStyle CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QWindowsVistaStyle
        DESTINATION "plugins/styles"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
endif()

install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/csound/plugins64-6.0"
    RUNTIME_DEPENDENCY_SET Justly_linked
)

set(Justly_runtime_regexes 
    "csnd"
    "csound"
    "FLAC"
    "icudata"
    "icui18n"
    "icuuc"
    "mp3lame"
    "mpg123"
    "ogg"
    "opus"
    "portaudio"
    "Qt6"
    "sndfile"
    "vorbis"
    "vorbisenc"
    "xcb-cursor"
)

install(RUNTIME_DEPENDENCY_SET Justly_linked
    PRE_EXCLUDE_REGEXES "."
    PRE_INCLUDE_REGEXES ${Justly_runtime_regexes}
    DIRECTORIES 
        "$<TARGET_FILE_DIR:CSound::csound64>"
        "$<TARGET_FILE_DIR:Qt6::Widgets>"
)

include(CPack)

mark_as_advanced(
    Justly_sources
    Justly_dependencies
    Justly_include_folders
    Justly_linked
    Justly_runtime_regexes
)