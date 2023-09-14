cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.9)

include(InstallRequiredSystemLibraries)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

option(JUSTLY_BUILD_TESTS "Build tests" OFF)
option(JUSTLY_TRACK_COVERAGE "Track coverage" ON)

find_package(CSound CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

find_package(Qt6Gui CONFIG REQUIRED)
list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

qt_standard_project_setup()

qt_add_library(justly_editor SHARED
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
if (LINUX)
    set_target_properties(justly_editor PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
target_include_directories(justly_editor PUBLIC
  "$<BUILD_INTERFACE:${Justly_SOURCE_DIR}/src>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(justly_editor PRIVATE
    "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
)
target_link_libraries(justly_editor PUBLIC
    CSound::libcsnd6
    Qt6::Widgets
    nlohmann_json_schema_validator
)
target_link_libraries(justly_editor PRIVATE
    CSound::csound64
)

qt_add_executable(Justly "src/main.cpp")
if (LINUX)
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()
target_link_libraries(Justly PRIVATE justly_editor Qt6::Widgets)
set_target_properties(Justly PROPERTIES MACOSX_BUNDLE TRUE)

if (JUSTLY_BUILD_TESTS)
    find_package(Qt6Test CONFIG REQUIRED)
    qt_add_executable(JustlyTests
        "src/Tester.h"
        "src/Tester.cpp"
        "src/test.cpp"
    )
    if (LINUX)
        set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    endif()
    set_target_properties(JustlyTests PROPERTIES MACOSX_BUNDLE TRUE)
    target_link_libraries(JustlyTests PUBLIC justly_editor Qt6::Widgets Qt6::Test)
    target_link_libraries(JustlyTests PRIVATE Qt6::Test)
    
    if (JUSTLY_TRACK_COVERAGE AND NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

endif()

install(TARGETS justly_editor
    RUNTIME_DEPENDENCY_SET Justly_linked
    BUNDLE DESTINATION "."
)
install(TARGETS Justly
    RUNTIME_DEPENDENCY_SET Justly_linked
    BUNDLE DESTINATION "."
)
if (JUSTLY_BUILD_TESTS)
    install(TARGETS JustlyTests
        RUNTIME_DEPENDENCY_SET JustlyTests_linked
        BUNDLE DESTINATION "."
    )
endif()
    
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md"
    DESTINATION "."
)

install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General.sf2" TYPE DATA)
install(FILES "${Justly_SOURCE_DIR}/share/MuseScore_General_License.md" TYPE DATA)
install(FILES "${Justly_SOURCE_DIR}/share/instruments.csv" TYPE DATA)

if (WIN32)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
    install(FILES "$<TARGET_FILE:nlohmann_json_schema_validator>" TYPE BIN)
endif()

# TODO: windows real-time instead?
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/csound/plugins64-6.0"
    RUNTIME_DEPENDENCY_SET Justly_linked
)
add_compile_definitions(REALTIME_PROVIDER="pa")

qt_generate_deploy_app_script(TARGET Justly
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if (JUSTLY_BUILD_TESTS)
    qt_generate_deploy_app_script(TARGET JustlyTests
        FILENAME_VARIABLE test_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${test_deploy_script})
endif()

if (LINUX)
    # although this will get deployed, do it manually as well
    # so we pick up the runtime dependency of xcb-cursor
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "plugins/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    if (Qt6Widgets_VERSION LESS "6.5")
        install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" DESTINATION TYPE BIN)

        find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
            DESTINATION "plugins/platformthemes"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()

    # x11 requires a physical screen
    # this allows tests to run without a physical screen
    if (JUSTLY_BUILD_TESTS)
        find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
            DESTINATION "plugins/platforms"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()
endif()

set(Justly_RUNTIME_DEPENDENCIES 
    "csnd"
    "csound"
    "FLAC"
    "icudata"
    "icui18n"
    "icuuc"
    "justly_editor"
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
mark_as_advanced(Justly_RUNTIME_DEPENDENCIES)

set(Justly_RUNTIME_DIRECTORIES
    "$<TARGET_FILE_DIR:CSound::csound64>"
    "$<TARGET_FILE_DIR:Qt6::Widgets>"
)
mark_as_advanced(Justly_RUNTIME_DIRECTORIES)

install(RUNTIME_DEPENDENCY_SET Justly_linked
    PRE_INCLUDE_REGEXES ${Justly_RUNTIME_DEPENDENCIES}
    PRE_EXCLUDE_REGEXES "."
    DIRECTORIES ${Justly_RUNTIME_DIRECTORIES}
)

if (JUSTLY_BUILD_TESTS)
    install(RUNTIME_DEPENDENCY_SET JustlyTests_linked
        PRE_INCLUDE_REGEXES ${Justly_RUNTIME_DEPENDENCIES}
        PRE_EXCLUDE_REGEXES "."
        DIRECTORIES ${Justly_RUNTIME_DIRECTORIES}
    )
endif()


include(CPack)
