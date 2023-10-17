cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11)

option(TRACK_COVERAGE "Track coverage" ON)

include(CheckCompilerFlag)
include(CheckLinkerFlag)
include(CPack)
include(InstallRequiredSystemLibraries)

# helpful for code editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# this means we don't have to set rpath for vcpkg non-plugin libraries
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# only will work on my windows boot
# list(APPEND CMAKE_PREFIX_PATH "C:/Users/brand/6.5.1/msvc2019_64")

find_path(FAST_CPP_CSV_PARSER_INCLUDE_DIR "fast-cpp-csv-parser/csv.h")

find_package(CSound CONFIG REQUIRED)
find_package(nlohmann_json_schema_validator REQUIRED)
find_package(Qt6Core CONFIG REQUIRED)
find_package(Qt6Gui CONFIG REQUIRED)
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

qt_standard_project_setup()

set(Justly_sources
    "src/editors/InstrumentEditor.h"
    "src/editors/InstrumentEditor.cpp"
    "src/editors/IntervalEditor.h"
    "src/editors/IntervalEditor.cpp"

    "src/main/Editor.h"
    "src/main/Editor.cpp"
    "src/main/MyDelegate.h"
    "src/main/MyDelegate.cpp"
    "src/main/MyView.h"
    "src/main/MyView.cpp"
    "src/main/Player.h"
    "src/main/Player.cpp"
    "src/main/Song.h"
    "src/main/Song.cpp"
    
    "src/metatypes/Instrument.h"
    "src/metatypes/Instrument.cpp"
    "src/metatypes/Interval.h"
    "src/metatypes/Interval.cpp"

    "src/models/ChordsModel.h"
    "src/models/ChordsModel.cpp"
    "src/models/InstrumentsModel.h"
    "src/models/InstrumentsModel.cpp"

    "src/notechord/Chord.h"
    "src/notechord/Chord.cpp"
    "src/notechord/Note.h"
    "src/notechord/Note.cpp"
    "src/notechord/NoteChord.h"
    "src/notechord/NoteChord.cpp"
    
    "src/commands/CellChange.h"
    "src/commands/CellChange.cpp"
    "src/commands/InsertRemoveChange.h"
    "src/commands/InsertRemoveChange.cpp"
    "src/commands/InsertEmptyChange.h"
    "src/commands/InsertEmptyChange.cpp"
    "src/commands/StartingValueChange.h"
    "src/commands/StartingValueChange.cpp"

    "src/utilities/JsonErrorHandler.h"
    "src/utilities/JsonErrorHandler.cpp"
    "src/utilities/SongIndex.h"
)

qt_add_executable(Justly MACOSX_BUNDLE
    ${Justly_sources}
    "src/main/main.cpp"
)
qt_add_executable(JustlyTests MACOSX_BUNDLE
    ${Justly_sources}
    "src/test/Tester.h"
    "src/test/Tester.cpp"
    "src/test/test.cpp"
)

target_link_libraries(JustlyTests PRIVATE Qt6::Test)

check_compiler_flag("CXX" "-Wall" has_all_warnings)
check_compiler_flag("CXX" "-Wextra" has_extra_warnings)
check_compiler_flag("CXX" "-g" has_debug_info)
check_compiler_flag("CXX" "-Og" has_debug_optimization)
check_linker_flag("CXX" "--coverage" has_link_coverage)
# doesn't work, dunno why
# check_compiler_flag("CXX" "--coverage" has_coverage)

if (TRACK_COVERAGE AND has_debug_optimization AND has_link_coverage)
    target_compile_options(JustlyTests PRIVATE "-Og")
    target_compile_options(JustlyTests PRIVATE "--coverage")
    target_link_options(JustlyTests PRIVATE "--coverage")
endif()

find_program(include_what_you_use NAMES "include-what-you-use")

# add license
install(FILES "${Justly_SOURCE_DIR}/LICENSE.md" DESTINATION ".")

if (APPLE)
    set(plugins_folder "PlugIns")
    set(data_folder "Resources")
else()
    set(plugins_folder "plugins")
    set(data_folder "share")
endif()

if (WIN32)
    # some runtime dependencies aren't picked up by windows
    # TODO: figure out why?
    find_package(FLAC CONFIG REQUIRED)
    install(FILES "$<TARGET_FILE:FLAC::FLAC>" TYPE BIN)
elseif (LINUX)
    # the plugins are submodules of Qt6::Gui
    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)

    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" TYPE BIN)
endif()

foreach(target_name IN ITEMS "Justly" "JustlyTests")

    set_property(TARGET ${target_name} PROPERTY "CXX_STANDARD" 17)
    set_property(TARGET ${target_name} PROPERTY "CXX_STANDARD_REQUIRED" "ON")

    target_link_libraries(${target_name} PRIVATE
        CSound::csound64
        CSound::libcsnd6
        Microsoft.GSL::GSL
        nlohmann_json_schema_validator
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    )

    target_include_directories(${target_name} PRIVATE
        "src"
        "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
    )

    target_compile_definitions(
        ${target_name} PRIVATE
        "PLUGINS_RELATIVE_PATH=\"../${plugins_folder}/csound\""
        "SOUNDFONT_RELATIVE_PATH=\"../${data_folder}/MuseScore_General.sf2\""
        "INSTRUMENTS_RELATIVE_PATH=\"../${data_folder}/instruments.csv\""
        "REALTIME_PROVIDER=\"pa\""
    )

    if (include_what_you_use)
        set_property(TARGET ${target_name} PROPERTY CXX_INCLUDE_WHAT_YOU_USE 
            "${include_what_you_use}"
        )
    endif()

    if (has_all_warnings)
        target_compile_options(${target_name} PRIVATE "-Wall")
    endif()

    if (has_extra_warnings)
        target_compile_options(${target_name} PRIVATE "-Wextra")
    endif()

    if (CMAKE_BUILD_TYPE STREQUAL Debug AND has_debug_info)
        target_compile_options(${target_name} PRIVATE "-g")
    endif()

    install(TARGETS ${target_name}
        RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
        BUNDLE DESTINATION "."
    )

    if (APPLE)
        set(data_subfolder "${target_name}.app/Contents/Resources")
        set(plugins_subfolder "${target_name}.app/Contents/PlugIns")
    else()
        set(data_subfolder "share")
        set(plugins_subfolder "plugins")
    endif()

    install(DIRECTORY "${Justly_SOURCE_DIR}/share/" DESTINATION ${data_subfolder})

    install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
        DESTINATION "${plugins_subfolder}/csound"
        RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
    )

    # Install the csound portaudio plugin
    # TODO: install other realtime plugins on other platforms
    install(IMPORTED_RUNTIME_ARTIFACTS CSound::rtpa
        DESTINATION "${plugins_subfolder}/csound"
        RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
    )
    
    qt_generate_deploy_app_script(TARGET ${target_name}
        FILENAME_VARIABLE "${target_name}_deploy_script"
        NO_UNSUPPORTED_PLATFORM_ERROR
    )

    install(SCRIPT ${${target_name}_deploy_script})

    if (LINUX)
        set_property(TARGET ${target_name} PROPERTY INSTALL_RPATH "$ORIGIN/../lib")

        # to pick up the xcb-cursor runtime dependency
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin
            DESTINATION "${plugins_subfolder}/platforms"
            RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
        )

        # to look nice
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QGtk3ThemePlugin
            DESTINATION "${plugins_subfolder}/platformthemes"
            RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
        )

        # this allows tests to run without a physical screen
        install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QOffscreenIntegrationPlugin
            DESTINATION "${plugins_subfolder}/platforms"
            RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
        )
    endif()

    install(RUNTIME_DEPENDENCY_SET "${target_name}_runtime_set"
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
            "nlohmann_json_schema_validator"
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
        DIRECTORIES 
            # vcpkg libraries
            "$<TARGET_FILE_DIR:CSound::csound64>"
            # qt libraries
            "$<TARGET_FILE_DIR:Qt6::Widgets>"
    )

    if (APPLE)
        set(bundle_folder "$<INSTALL_PREFIX>/${target_name}.app")

        install(CODE "
            include(BundleUtilities)

            # find plugins
            file(GLOB ${target_name}_plugins
                \"${bundle_folder}/Contents/PlugIns/*/*.dylib\"
            )

            # find framework folders
            file(GLOB ${target_name}_frameworks
                \"${bundle_folder}/Contents/Frameworks/*.framework\"
            )

            fixup_bundle(
                ${bundle_folder}
                \"\${${target_name}_plugins}\"
                \"\${${target_name}_frameworks}\"
            )
        ")
    endif()
endforeach()

