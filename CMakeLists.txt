cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11)

include(InstallRequiredSystemLibraries)

option(Justly_TEST OFF)

# helpful for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# only will work on my windows boot but
list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

find_package(CSound CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

set(SOURCES
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
mark_as_advanced(SOURCES)

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

if (Justly_TEST)
    find_package(Qt6Test CONFIG REQUIRED)

    qt_add_executable(Justly MACOSX_BUNDLE
        ${SOURCES}
        "src/Tester.h"
        "src/Tester.cpp"
        "src/test.cpp"
    )
    list(APPEND DEPENDENCIES Qt6::Test)
    list(APPEND DEPENDENCY_FOLDERS "$<TARGET_FILE_DIR:Qt6::Test>")

    # coverage isn't available on MSVC
    if (NOT MSVC)
        target_compile_options(Justly PRIVATE "-g" "--coverage" "-O0")
        target_link_libraries(Justly PRIVATE "--coverage")
    endif()
else()
    qt_add_executable(Justly MACOSX_BUNDLE ${SOURCES} "src/main.cpp")
endif()

target_include_directories(Justly PRIVATE
    "${Justly_SOURCE_DIR}/src"
    "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
)

target_link_libraries(Justly PRIVATE ${DEPENDENCIES})

if (LINUX)
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
endif()

if (APPLE)
    target_compile_definitions(
        Justly PRIVATE
        PLUGINS_RELATIVE_PATH="../Plugins/csound"
        SOUNDFONT_RELATIVE_PATH="../Resources/MuseScore_General.sf2"
        INSTRUMENTS_RELATIVE_PATH="../Resources/instruments.csv"
    )
    set(CMAKE_INSTALL_DATADIR "Justly.app/Contents/Resources")
    set(PLUGINS_SUBFOLDER "Justly.app/Contents/PlugIns")
else()
    target_compile_definitions(
        Justly PRIVATE
        PLUGINS_RELATIVE_PATH="../plugins/csound"
        SOUNDFONT_RELATIVE_PATH="../share/MuseScore_General.sf2"
        INSTRUMENTS_RELATIVE_PATH="../share/instruments.csv"
    )
    set(PLUGINS_SUBFOLDER "plugins")
endif()

mark_as_advanced(PLUGINS_SUBFOLDER)

install(TARGETS Justly
    RUNTIME_DEPENDENCY_SET Justly_linked
    BUNDLE DESTINATION "."
)

# add license
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md" DESTINATION ".")

# install the instrument data
install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)

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
    DESTINATION "${PLUGINS_SUBFOLDER}/csound"
    RUNTIME_DEPENDENCY_SET Justly_linked
)
add_compile_definitions(REALTIME_PROVIDER="pa")

# deploy Qt
qt_generate_deploy_app_script(TARGET Justly
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if (LINUX)
    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" DESTINATION TYPE BIN)

    # the plugins are submodules of Qt6::Gui
    find_package(Qt6Gui CONFIG REQUIRED)
    list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

    # to pick up the xcb-cursor runtime dependency
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
        DESTINATION "${PLUGINS_SUBFOLDER}/platforms"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    # to look nice
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
        DESTINATION "${PLUGINS_SUBFOLDER}/platformthemes"
        RUNTIME_DEPENDENCY_SET Justly_linked
    )

    if (Justly_TEST)
        # this allows tests to run without a physical screen
        find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
            DESTINATION "${PLUGINS_SUBFOLDER}/platforms"
            RUNTIME_DEPENDENCY_SET Justly_linked
        )
    endif()
endif()

install(RUNTIME_DEPENDENCY_SET Justly_linked
    PRE_INCLUDE_REGEXES
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
    PRE_EXCLUDE_REGEXES "."
    # folders to look in for platforms without an rpath
    DIRECTORIES ${DEPENDENCY_FOLDERS}
)

if (APPLE)
    # fixup the bundle
    set(INSTALLED_BUNDLE_FOLDER "$<INSTALL_PREFIX>/Justly.app")
    mark_as_advanced(INSTALLED_BUNDLE_FOLDER)

    install(CODE "
        include(BundleUtilities)

        # find plugins
        file(GLOB INSTALLED_PLUGINS \"${INSTALLED_BUNDLE_FOLDER}/Contents/PlugIns/*/*.dylib\")
        # find framework folders
        file(GLOB INSTALLED_FRAMEWORK_FOLDERS \"${INSTALLED_BUNDLE_FOLDER}/Contents/Frameworks/*.framework\")

        fixup_bundle(
            \"$<INSTALL_PREFIX>/Justly.app\"
            \"\${INSTALLED_PLUGINS}\"
            \"\${INSTALLED_FRAMEWORK_FOLDERS}\"
        )
    ")
endif()

include(CPack)
