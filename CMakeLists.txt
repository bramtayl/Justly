cmake_minimum_required(VERSION 3.24)

project(Justly VERSION 0.3.11)

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
find_package(Qt6Test CONFIG REQUIRED)
find_package(Qt6Widgets CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

qt_standard_project_setup()

set(Justly_sources
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

qt_add_executable(Justly MACOSX_BUNDLE
    ${Justly_sources}
    "src/main.cpp"
)
qt_add_executable(JustlyTests MACOSX_BUNDLE
    ${Justly_sources}
    "src/Tester.h"
    "src/Tester.cpp"
    "src/test.cpp"
)

target_link_libraries(JustlyTests PRIVATE Qt6::Test)

# coverage isn't available on MSVC
if (NOT MSVC)
    target_compile_options(JustlyTests PRIVATE "-g" "--coverage" "-O0")
    target_link_libraries(JustlyTests PRIVATE "--coverage")
endif()

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
    find_package(Qt6Gui CONFIG REQUIRED)
    list(APPEND CMAKE_PREFIX_PATH ${Qt6Gui_DIR})

    find_package(Qt6QXcbIntegrationPlugin CONFIG REQUIRED)
    find_package(Qt6QGtk3ThemePlugin CONFIG REQUIRED)
    find_package(Qt6QOffscreenIntegrationPlugin CONFIG REQUIRED)

    # deployment won't work on Qt < 6.5 so we might need this
    install(FILES "${Justly_SOURCE_DIR}/bin/qt.conf" TYPE BIN)
endif()

foreach(target_name IN ITEMS "Justly" "JustlyTests")
    target_link_libraries(${target_name} PRIVATE
        CSound::csound64
        CSound::libcsnd6
        nlohmann_json_schema_validator
        Qt6::Widgets
    )

    target_include_directories(${target_name} PRIVATE
        "${Justly_SOURCE_DIR}/src"
        "${FAST_CPP_CSV_PARSER_INCLUDE_DIR}"
    )

    target_compile_definitions(
        ${target_name} PRIVATE
        "PLUGINS_RELATIVE_PATH=\"../${plugins_folder}/csound\""
        "SOUNDFONT_RELATIVE_PATH=\"../${data_folder}/MuseScore_General.sf2\""
        "INSTRUMENTS_RELATIVE_PATH=\"../${data_folder}/instruments.csv\""
        "REALTIME_PROVIDER=\"pa\""
    )

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
        set_target_properties(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

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

include(CPack)
