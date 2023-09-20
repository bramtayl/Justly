cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11)

include(InstallRequiredSystemLibraries)

option(Justly_TEST OFF)

# helpful for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# only will work on my windows boot
# list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

find_package(CSound CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

set(SOURCES
    "src/Editor.cpp"
    "src/Instrument.cpp"
    "src/JsonErrorHandler.cpp"
    "src/Player.cpp"
    "src/Song.cpp"
    "src/StableIndex.cpp"
    "src/TreeNode.cpp"
    "src/utilities.cpp"

    "src/editors/IntervalEditor.cpp"
    "src/editors/ShowSlider.cpp"

    "src/metatypes/Interval.cpp"
    "src/metatypes/SuffixedNumber.cpp"

    "src/models/ChordsModel.cpp"
    "src/models/InstrumentsModel.cpp"

    "src/delegates/ComboBoxDelegate.cpp"
    "src/delegates/IntervalDelegate.cpp"
    "src/delegates/ShowSliderDelegate.cpp"
    "src/delegates/SpinBoxDelegate.cpp"
    "src/notechord/Chord.cpp"
    "src/notechord/Note.cpp"
    "src/notechord/NoteChord.cpp"
    "src/commands/CellChange.cpp"
    "src/commands/InsertChange.cpp"
    "src/commands/InsertNewChange.cpp"
    "src/commands/RemoveChange.cpp"
    "src/commands/StartingInstrumentChange.cpp"
    "src/commands/StartingKeyChange.cpp"
    "src/commands/StartingTempoChange.cpp"
    "src/commands/StartingVolumeChange.cpp"
)
mark_as_advanced(SOURCES)

set(INCLUDE_DIRS
    "${Justly_SOURCE_DIR}/src"
    "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
)
mark_as_advanced(INCLUDE_DIRS)

qt_standard_project_setup()

set(DEPENDENCIES
    CSound::csound64
    CSound::libcsnd6
    nlohmann_json_schema_validator
    Qt6::Widgets
)
mark_as_advanced(DEPENDENCIES)

set(DEPENDENCY_FOLDERS 
    "$<TARGET_FILE_DIR:CSound::csound64>"
    "$<TARGET_FILE_DIR:CSound::libcsnd6>"
    "$<TARGET_FILE_DIR:nlohmann_json_schema_validator>"
    "$<TARGET_FILE_DIR:Qt6::Widgets>"
)
mark_as_advanced(DEPENDENCY_FOLDERS)

set(RUNTIME_DEPENDENCY_REGEXES
    "csnd"
    "csound"
    "FLAC"
    # on linux, Qt needs an old ICU version
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
    # not shipped by default on Ubuntu so add it
    "xcb-cursor"
)
mark_as_advanced(RUNTIME_DEPENDENCY_REGEXES)

if (APPLE)
    set(PLUGINS_RELATIVE_PATH "../Plugins/csound")
    set(SOUNDFONT_RELATIVE_PATH "../Resources/MuseScore_General.sf2")
    set(INSTRUMENTS_RELATIVE_PATH "../Resources/instruments.csv")
    set(Justly_DATA_SUBFOLDER "Justly.app/Contents/Resources")
    set(JustlyTests_DATA_SUBFOLDER "JustlyTests.app/Contents/Resources")
    set(Justly_PLUGINS_SUBFOLDER "Justly.app/Contents/PlugIns")
    set(JustlyTests_PLUGINS_SUBFOLDER "JustlyTests.app/Contents/PlugIns")
else()
    set(PLUGINS_RELATIVE_PATH "../plugins/csound")
    set(SOUNDFONT_RELATIVE_PATH "../share/MuseScore_General.sf2")
    set(INSTRUMENTS_RELATIVE_PATH "../share/instruments.csv")
    set(Justly_DATA_SUBFOLDER "share")
    set(JustlyTests_DATA_SUBFOLDER "share")
    set(Justly_PLUGINS_SUBFOLDER "plugins")
    set(JustlyTests_PLUGINS_SUBFOLDER "plugins")
endif()

mark_as_advanced(
    PLUGINS_RELATIVE_PATH
    SOUNDFONT_RELATIVE_PATH
    INSTRUMENTS_RELATIVE_PATH
    Justly_DATA_SUBFOLDER
    JustlyTests_DATA_SUBFOLDER
    Justly_PLUGINS_SUBFOLDER
    JustlyTests_PLUGINS_SUBFOLDER
)

qt_add_executable(Justly MACOSX_BUNDLE
    ${SOURCES}
    "src/main.cpp"
)
qt_add_executable(JustlyTests MACOSX_BUNDLE
    ${SOURCES}
    "src/Tester.h"
    "src/Tester.cpp"
    "src/test.cpp"
)

target_include_directories(Justly PRIVATE ${INCLUDE_DIRS})
target_include_directories(JustlyTests PRIVATE ${INCLUDE_DIRS})

target_link_libraries(Justly PRIVATE ${DEPENDENCIES})
target_link_libraries(JustlyTests PRIVATE ${DEPENDENCIES} Qt6::Test)

# coverage isn't available on MSVC
if (NOT MSVC)
    target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
    target_link_libraries(JustlyTests PRIVATE "--coverage")
endif()

if (LINUX)
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()


target_compile_definitions(
    Justly PRIVATE
    "PLUGINS_RELATIVE_PATH=\"${PLUGINS_RELATIVE_PATH}\""
    "SOUNDFONT_RELATIVE_PATH=\"${SOUNDFONT_RELATIVE_PATH}\""
    "INSTRUMENTS_RELATIVE_PATH=\"${INSTRUMENTS_RELATIVE_PATH}\""
    "REALTIME_PROVIDER=\"pa\""
)
target_compile_definitions(
    JustlyTests PRIVATE
    "PLUGINS_RELATIVE_PATH=\"${PLUGINS_RELATIVE_PATH}\""
    "SOUNDFONT_RELATIVE_PATH=\"${SOUNDFONT_RELATIVE_PATH}\""
    "INSTRUMENTS_RELATIVE_PATH=\"${INSTRUMENTS_RELATIVE_PATH}\""
    "REALTIME_PROVIDER=\"pa\""
)

install(TARGETS Justly
    RUNTIME_DEPENDENCY_SET Justly_linked
    BUNDLE DESTINATION "."
)
install(TARGETS JustlyTests
    RUNTIME_DEPENDENCY_SET JustlyTests_linked
    BUNDLE DESTINATION "."
)

# add license
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md" DESTINATION ".")

# install the instrument data
install(DIRECTORY "${Justly_SOURCE_DIR}/share/" DESTINATION "${Justly_DATA_SUBFOLDER}")
install(DIRECTORY "${Justly_SOURCE_DIR}/share/" DESTINATION "${JustlyTests_DATA_SUBFOLDER}")

if (WIN32)
    # some runtime dependencies aren't picked up by windows
    # TODO: figure out why?
    find_package(FLAC CONFIG REQUIRED)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)

    install(FILES "$<TARGET_FILE:nlohmann_json_schema_validator>" TYPE BIN)
endif()

# Install the csound portaudio plugin
# TODO: install other realtime plugins on other platforms
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "${Justly_PLUGINS_SUBFOLDER}/csound"
    RUNTIME_DEPENDENCY_SET Justly_linked
)
install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
    DESTINATION "${JustlyTests_PLUGINS_SUBFOLDER}/csound"
    RUNTIME_DEPENDENCY_SET JustlyTests_linked
)

# deploy Qt
qt_generate_deploy_app_script(TARGET Justly
    FILENAME_VARIABLE Justly_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
qt_generate_deploy_app_script(TARGET JustlyTests
    FILENAME_VARIABLE JustlyTests_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${Justly_DEPLOY_SCRIPT})
install(SCRIPT ${JustlyTests_DEPLOY_SCRIPT})

if (LINUX)
    # this will all work for both Justly and JustlyTests

    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" TYPE BIN)

    # the plugins are submodules of Qt6::Gui
    find_package(Qt6Gui CONFIG REQUIRED)
    list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

    # to pick up the xcb-cursor runtime dependency
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "${Justly_PLUGINS_SUBFOLDER}/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "${JustlyTests_PLUGINS_SUBFOLDER}/platforms"
        RUNTIME_DEPENDENCY_SET JustlyTests_linked
    )

    # to look nice
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "${Justly_PLUGINS_SUBFOLDER}/platformthemes"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "${JustlyTests_PLUGINS_SUBFOLDER}/platformthemes"
        RUNTIME_DEPENDENCY_SET JustlyTests_linked
    )

    # this allows tests to run without a physical screen
    find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
        DESTINATION "${Justly_PLUGINS_SUBFOLDER}/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
        DESTINATION "${JustlyTests_PLUGINS_SUBFOLDER}/platforms"
        RUNTIME_DEPENDENCY_SET JustlyTests_linked
    )
endif()

install(RUNTIME_DEPENDENCY_SET Justly_linked
    PRE_INCLUDE_REGEXES ${RUNTIME_DEPENDENCY_REGEXES}
    PRE_EXCLUDE_REGEXES "."
    # folders to look in for platforms without an rpath
    DIRECTORIES 
        ${DEPENDENCY_FOLDERS}
)
install(RUNTIME_DEPENDENCY_SET JustlyTests_linked
    PRE_INCLUDE_REGEXES ${RUNTIME_DEPENDENCY_REGEXES}
    PRE_EXCLUDE_REGEXES "."
    # folders to look in for platforms without an rpath
    DIRECTORIES 
        ${DEPENDENCY_FOLDERS}
        "$<TARGET_FILE_DIR:Qt6::Test>"
)

if (APPLE)
    # fixup the bundle
    set(Justly_INSTALLED_BUNDLE_FOLDER "$<INSTALL_PREFIX>/Justly.app")
    set(JustlyTests_INSTALLED_BUNDLE_FOLDER "$<INSTALL_PREFIX>/JustlyTests.app")
    mark_as_advanced(
        Justly_INSTALLED_BUNDLE_FOLDER
        Justly_INSTALLED_BUNDLE_FOLDER
    )

    install(CODE "
        include(BundleUtilities)

        # find plugins
        file(GLOB Justly_INSTALLED_PLUGINS \"${Justly_INSTALLED_BUNDLE_FOLDER}/Contents/PlugIns/*/*.dylib\")
        file(GLOB JustlyTests_INSTALLED_PLUGINS \"${JustlyTests_INSTALLED_BUNDLE_FOLDER}/Contents/PlugIns/*/*.dylib\")

        # find framework folders
        file(GLOB Justly_INSTALLED_FRAMEWORK_FOLDERS \"${Justly_INSTALLED_BUNDLE_FOLDER}/Contents/Frameworks/*.framework\")
        file(GLOB JustlyTests_INSTALLED_FRAMEWORK_FOLDERS \"${JustlyTests_INSTALLED_BUNDLE_FOLDER}/Contents/Frameworks/*.framework\")

        fixup_bundle(
            \"${Justly_INSTALLED_BUNDLE_FOLDER}\"
            \"\${Justly_INSTALLED_PLUGINS}\"
            \"\${Justly_INSTALLED_FRAMEWORK_FOLDERS}\"
        )
        fixup_bundle(
            \"${JustlyTests_INSTALLED_BUNDLE_FOLDER}\"
            \"\${JustlyTests_INSTALLED_PLUGINS}\"
            \"\${JustlyTests_INSTALLED_FRAMEWORK_FOLDERS}\"
        )
    ")
endif()

include(CPack)
