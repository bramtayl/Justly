cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.10)

# sounds useful?
include(InstallRequiredSystemLibraries)

# helpful for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# needed to add symbols to dlls automatically
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# only will work on my windows boot but
list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

option(JUSTLY_BUILD_TESTS "Build tests" OFF)

find_package(CSound CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

find_package(Qt6Gui CONFIG REQUIRED)
list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

if (APPLE)
    add_compile_definitions(
        PLUGINS_RELATIVE_PATH="../Plugins/csound"
        SOUNDFONT_RELATIVE_PATH="../Resources/MuseScore_General.sf2"
    )
else()
    add_compile_definitions(
        PLUGINS_RELATIVE_PATH="../plugins/csound"
        SOUNDFONT_RELATIVE_PATH="../share/MuseScore_General.sf2"
    )
endif()

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
    
    if (NOT MSVC)
        target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
        target_link_libraries(JustlyTests PRIVATE "--coverage")
    endif()

endif()

if (NOT APPLE)
    install(TARGETS justly_editor
        RUNTIME_DEPENDENCY_SET Justly_linked
        BUNDLE DESTINATION "."
    )
endif()

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

# add license
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md" DESTINATION ".")

if (APPLE)
    install(
        DIRECTORY "${Justly_SOURCE_DIR}/share/"
        DESTINATION "Justly.app/Contents/Resources"
    )
    if (JUSTLY_BUILD_TESTS)
        install(
            DIRECTORY "${Justly_SOURCE_DIR}/share/"
            DESTINATION "JustlyTests.app/Contents/Resources"
        )
    endif()
else()
    install(
        DIRECTORY "${Justly_SOURCE_DIR}/share/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}"
    )
endif()

if (WIN32)
    # some runtime dependencies aren't picked up by windows
    # TODO: figure out why?
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
    install(FILES "$<TARGET_FILE:nlohmann_json_schema_validator>" TYPE BIN)
endif()

# TODO: allow other realtime providers?
if (APPLE)
    install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
        DESTINATION "Justly.app/Contents/Plugins/csound"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    if (JUSTLY_BUILD_TESTS)
        install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
            DESTINATION "JustlyTests.app/Contents/Plugins/csound"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()
else()
    install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
        DESTINATION "plugins/csound"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
endif()
add_compile_definitions(REALTIME_PROVIDER="pa")

qt_generate_deploy_app_script(TARGET Justly
    # TODO: change to OUTPUT_FILE once >= 6.5 becomes debian default
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
        # can't deploy linux on previous versions
        # TODO: remove once >= 6.5 becomes debian default
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
    # Qt needs its own ICU versions apparently
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
    # this doesn't come with ubuntu by default so make sure it's available
    "xcb-cursor"
)
mark_as_advanced(Justly_RUNTIME_DEPENDENCIES)

# need this for platforms without an rpath
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

if (APPLE)
    INSTALL(
        CODE "
            include(BundleUtilities)
            fixup_bundle(\"Justly\" \"justly_editor;CSound::rtpa\" \"$<TARGET_FILE_DIR:CSound::csound64>;$<TARGET_FILE_DIR:Qt6::Widgets>\")
        "
        COMPONENT Runtime
    )
    if (JUSTLY_BUILD_TESTS)
        INSTALL(
            CODE "
                include(BundleUtilities)
                fixup_bundle(\"JustlyTests\" \"justly_editor;CSound::rtpa\" \"$<TARGET_FILE_DIR:CSound::csound64>;$<TARGET_FILE_DIR:Qt6::Widgets>\")
            " 
            COMPONENT Runtime
        )
    endif()
endif()



include(CPack)
