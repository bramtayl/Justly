# 3.25 for the LINUX variable
cmake_minimum_required(VERSION 3.25)

# on windows, pass the path to Qt, e.g. -DQt6_ROOT=C:/Qt/6.10.1/msvc2022_64

# must be set before project(), otherwise CMake defaults to build version
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

project(Justly VERSION 1.2.0 LANGUAGES CXX)

# MuseScore's MS Basic soundfont, too big to commit to git, so users put it
# in the share folder (its license, licenses/MS_Basic.md, is committed)
set(soundfont_url "https://github.com/musescore/MuseScore/raw/v4.7.4/share/sound/MS%20Basic.sf3")
set(soundfont_file "${Justly_SOURCE_DIR}/share/MS_Basic.sf3")
if (NOT EXISTS "${soundfont_file}")
    message(FATAL_ERROR "Download ${soundfont_url} to ${soundfont_file}")
endif()
file(SHA256 "${soundfont_file}" soundfont_hash)
if (NOT soundfont_hash STREQUAL "5ea2375e8bd7d8e71def1036978c1621e85b66934169b6a2744b27b9b3c2d99c")
    message(FATAL_ERROR "${soundfont_file} isn't ${soundfont_url}")
endif()

# on Linux, the package manager (or Flatpak) provides Qt, FluidSynth, and a
# python with music21, so Justly bundles none of them, and its share folder
# goes in share/Justly alongside other apps'. On MacOS and Windows, Justly
# bundles them all
if (APPLE)
    set(share_sub_folder "Resources")
elseif (LINUX)
    set(share_sub_folder "share/Justly")
else()
    set(share_sub_folder "share")
endif()

set(music21_version "10.5.0")

# importing a score runs share/import_score.py, which reads it with music21
if (LINUX)
    # prefers the python in .venv, so it doesn't need activating, unless one
    # is passed with -DPython3_EXECUTABLE
    set(venv_python "${Justly_SOURCE_DIR}/.venv/bin/python")
    if (NOT DEFINED Python3_EXECUTABLE AND EXISTS "${venv_python}")
        set(Python3_EXECUTABLE "${venv_python}")
    endif()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import music21; print(music21.__version__)"
        OUTPUT_VARIABLE system_music21_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE music21_missing
    )
    if (music21_missing OR NOT system_music21_version STREQUAL music21_version)
        message(FATAL_ERROR "${Python3_EXECUTABLE} needs music21 ${music21_version}. See Building from source in README.md")
    endif()
else()
    # a python bundled with Justly. It has to be a python-build-standalone
    # build, which works from wherever it's copied. Users install one with uv
    # into the python folder, apart from any other python, and music21 into it
    set(python_install_folder "${Justly_SOURCE_DIR}/python")
    set(python_instructions "See Building from source in README.md for how to install it")
    # uv also adds a cpython-3.13-<platform> link to the newest 3.13, which this
    # leaves out
    file(GLOB python_source_folders LIST_DIRECTORIES true
        "${python_install_folder}/cpython-3.13.*")
    list(LENGTH python_source_folders python_count)
    if (NOT python_count EQUAL 1)
        message(FATAL_ERROR "Expected one python 3.13 in ${python_install_folder}, found ${python_count}. ${python_instructions}")
    endif()
    if (WIN32)
        set(python_source "${python_source_folders}/python.exe")
    else()
        set(python_source "${python_source_folders}/bin/python3")
    endif()

    # packages no other package needs. Everything else is one of music21's
    # dependencies
    execute_process(
        COMMAND "${python_source}" -m pip list
            --not-required --format=freeze --disable-pip-version-check
        OUTPUT_VARIABLE top_level_packages
        COMMAND_ERROR_IS_FATAL ANY
    )
    string(REPLACE "\r" "" top_level_packages "${top_level_packages}")
    # pip itself comes with python
    string(REGEX REPLACE "(^|\n)pip==[^\n]*" "" top_level_packages "${top_level_packages}")
    string(STRIP "${top_level_packages}" top_level_packages)
    if (NOT top_level_packages STREQUAL "music21==${music21_version}")
        string(REPLACE "\n" ", " top_level_packages "${top_level_packages}")
        message(FATAL_ERROR "The python in ${python_install_folder} needs music21 ${music21_version} and nothing else, but has: ${top_level_packages}. ${python_instructions}")
    endif()

    # on Windows, straight into the build tree's share folder, where the
    # targets find it. Mac bundles have their own share folders, so each
    # target copies it in after it builds
    if (APPLE)
        set(python_folder "${CMAKE_BINARY_DIR}/python")
    else()
        set(python_folder "${CMAKE_BINARY_DIR}/share/python")
    endif()
    if (WIN32)
        set(bundled_python "${python_folder}/python.exe")
    else()
        set(bundled_python "${python_folder}/bin/python3")
    endif()

    # copied again, over the whole folder, only when it's a different python or
    # music21
    set(python_copy "${python_source_folders} music21 ${music21_version}")
    if (NOT python_copy STREQUAL last_python_copy
        OR NOT EXISTS "${bundled_python}")
        message(STATUS "Copying ${python_source_folders}")
        file(REMOVE_RECURSE "${python_folder}")
        file(COPY "${python_source_folders}/" DESTINATION "${python_folder}")
        set(last_python_copy "${python_copy}" CACHE INTERNAL "")
    endif()
endif()

include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg Widgets)
if (BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    # headers without matching .cpp files won't get checked by default
    # so specify all header dirs (iwyu's * also matches across /)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
        include-what-you-use
        -Xiwyu --mapping_file=${Justly_SOURCE_DIR}/map.imp
        -Xiwyu --check_also=${Justly_SOURCE_DIR}/library/*
        -Xiwyu --check_also=${Justly_SOURCE_DIR}/executable/*
        -Xiwyu --check_also=${Justly_SOURCE_DIR}/tests/*
    )
endif()

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall" "-Wextra" "-Wpedantic"
        "$<$<OR:$<CONFIG:Debug>,$<BOOL:${TRACK_COVERAGE}>>:-Og>"
    )
endif()

if (TRACK_COVERAGE)
    add_compile_options("--coverage")
    add_link_options("--coverage")
endif()

# add an extra subfolder so that we can put a share folder next to it
# to mirror the install tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

qt_add_library(JustlyLibrary STATIC)
qt_add_executable(Justly)
set(app_targets Justly)
if (BUILD_TESTS)
    enable_testing()
    qt_add_executable(JustlyTests)
    list(APPEND app_targets JustlyTests)
    if (APPLE)
        # need to call before qt_generate_deploy_app_script()
        set_target_properties(JustlyTests PROPERTIES MACOSX_BUNDLE ON)
    endif()
endif()

add_subdirectory("library")
add_subdirectory("executable")

# install the share folder, and also put it into the build dir so the target works there too
function(justly_install_share share_folder bundle_target)
    if (APPLE)
        set(install_sub_folder "${bundle_target}.app/Contents/")
    else()
        set(install_sub_folder "")
    endif()

    install(DIRECTORY "${share_folder}" DESTINATION "${install_sub_folder}${share_sub_folder}")
    # next to the third-party licenses in share/licenses
    install(FILES "${Justly_SOURCE_DIR}/LICENSE.md"
        DESTINATION "${install_sub_folder}${share_sub_folder}/licenses"
        RENAME "Justly.md"
    )
    if (NOT LINUX)
        install(DIRECTORY "${python_folder}/"
            DESTINATION "${install_sub_folder}${share_sub_folder}/python"
            USE_SOURCE_PERMISSIONS
        )
    endif()
    if (APPLE)
        add_custom_command(TARGET ${bundle_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${python_folder}" "$<TARGET_FILE_DIR:${bundle_target}>/../${share_sub_folder}/python"
        )
    endif()

    add_custom_command(TARGET ${bundle_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${share_folder}" "$<TARGET_FILE_DIR:${bundle_target}>/../${share_sub_folder}"
    )
endfunction()

foreach(app_target ${app_targets})
    # need to call before qt_generate_deploy_app_script
    install(TARGETS ${app_target}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )

    # on Linux, the installed targets use the system's libraries where they
    # are, so there's nothing to deploy
    if (NOT LINUX)
        qt_generate_deploy_app_script(
            TARGET ${app_target}
            OUTPUT_SCRIPT deploy_script
            NO_TRANSLATIONS
        )
        install(SCRIPT ${deploy_script})
    endif()

    justly_install_share("${Justly_SOURCE_DIR}/share/" ${app_target})
endforeach()

# so desktops can find and launch Justly
if (LINUX)
    set(app_id "io.github.bramtayl.Justly")
    install(FILES "${Justly_SOURCE_DIR}/linux/${app_id}.desktop"
        DESTINATION "share/applications")
    install(FILES "${Justly_SOURCE_DIR}/linux/${app_id}.metainfo.xml"
        DESTINATION "share/metainfo")
    # file types, so desktops can open songs and scores with Justly
    install(FILES "${Justly_SOURCE_DIR}/linux/${app_id}.mime.xml"
        DESTINATION "share/mime/packages"
        RENAME "${app_id}.xml")
    install(FILES "${Justly_SOURCE_DIR}/share/Justly.svg"
        DESTINATION "share/icons/hicolor/scalable/apps"
        RENAME "${app_id}.svg")
endif()

if (WIN32)
    # normalize path separators
    cmake_policy(SET CMP0207 NEW)

    set(runtime_dep_executables)
    foreach(app_target ${app_targets})
        list(APPEND runtime_dep_executables "$<TARGET_FILE:${app_target}>")
    endforeach()

    # this is done by hand with file(GET_RUNTIME_DEPENDENCIES) rather than
    # install(RUNTIME_DEPENDENCY_SET), because some optional Windows OS
    # components (attestation, performance-diagnostics, trust-decision
    # sensors -- e.g. AzureAttestManager.dll, PdmUtilities.dll) show up as
    # dependencies but aren't necessarily present as real files on any
    # given machine, including the CI runner, no matter how the exclude
    # regexes are written; install(RUNTIME_DEPENDENCY_SET) treats any
    # unresolved dependency as an unconditional hard error with no way to
    # opt out, so it can't tolerate that. Doing it by hand lets us just
    # warn and move on instead, which is fine since those aren't ones we
    # could bundle even if we wanted to.
    install(CODE "
        # cmake_policy() at configure time doesn't carry into this script --
        # it runs as its own separate invocation at install time -- so it
        # has to be set again here, otherwise file(GET_RUNTIME_DEPENDENCIES)
        # matches POST_EXCLUDE_REGEXES against paths with mixed slash
        # direction and case, which can dodge the regex below entirely
        cmake_policy(SET CMP0207 NEW)
        file(GET_RUNTIME_DEPENDENCIES
            RESOLVED_DEPENDENCIES_VAR resolved_deps
            UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
            EXECUTABLES ${runtime_dep_executables}
            DIRECTORIES \"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin\"
            PRE_EXCLUDE_REGEXES \"^api-ms-\" \"^ext-ms-\" \"^qt6\"
            POST_EXCLUDE_REGEXES \".*/[Ss]ystem32/.*[.]dll\"
        )
        if(unresolved_deps)
            message(WARNING \"Skipping unresolved runtime dependencies (assumed to be optional Windows OS components not bundled anyway): \${unresolved_deps}\")
        endif()
        file(INSTALL \${resolved_deps} DESTINATION \"\${CMAKE_INSTALL_PREFIX}/bin\")
    ")
endif()

if (BUILD_TESTS)
    # qt_import_plugins won't work for platform plugins, so where Qt is
    # bundled, the offscreen plugin needs to be deployed manually. On Linux,
    # the system's Qt finds its own
    if (APPLE)
        set(platform_plugin_subfolder "JustlyTests.app/Contents/PlugIns/platforms")
    elseif (WIN32)
        set(platform_plugin_subfolder "bin/platforms")
    endif()

    if (NOT LINUX)
        set(platform_plugin_folder "\${CMAKE_INSTALL_PREFIX}/${platform_plugin_subfolder}")
        set(platform_plugin_path "${platform_plugin_folder}/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>")

        # install(IMPORTED_RUNTIME_ARTIFACTS) doesn't reliably resolve this
        # plugin's location, so copy it manually via $<TARGET_FILE:...>
        install(CODE "
            file(MAKE_DIRECTORY \"${platform_plugin_folder}\")
            file(COPY_FILE
                \"$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>\"
                \"${platform_plugin_path}\"
            )
        ")
    endif()

    if (APPLE)
        # set the rpath for the platform plugin
        install(CODE "
            execute_process(COMMAND install_name_tool -add_rpath
                \"@loader_path/../../Frameworks\"
                \"${platform_plugin_path}\"
            )
        ")
    endif()

    add_subdirectory("tests")


    justly_install_share("${Justly_SOURCE_DIR}/tests/share/" JustlyTests)

    # one per build dir, so separate builds can't see each other's leftover files
    set(test_install_folder "${CMAKE_BINARY_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests Justly JustlyTests)

    set(run_tests_subfolder "bin")
    set(run_tests_args "")
    if (APPLE)
        set(run_tests_subfolder "JustlyTests.app/Contents/MacOS")
    elseif (WIN32)
        # test output doesn't get piped to stdout on windows
        set(run_tests_args -o test_output.txt,txt)
    endif()

    add_test(NAME run_tests COMMAND "${test_install_folder}/${run_tests_subfolder}/JustlyTests" ${run_tests_args})
    # JustlyTests creates real widgets, which needs a display -- CI runners
    # don't have one, so force Qt's software-rendered offscreen backend
    # (see the Qt6QOffscreenIntegrationPlugin find_package() call above)
    set_tests_properties(run_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    if (APPLE)
        # make sure the bundle can open
        add_test(NAME bundle_opens
            COMMAND open -W -a "${test_install_folder}/JustlyTests.app")
    endif()
endif()

# an installer, with the WIX generator, that adds Justly to the start menu
# and opens songs and scores with it. Needs the wix .NET tool and its UI
# extension, see .github/workflows/workflow.yaml
if (WIN32)
    set(CPACK_PACKAGE_VENDOR "Brandon Taylor")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Justly")
    set(CPACK_PACKAGE_EXECUTABLES "Justly" "Justly")
    set(CPACK_WIX_VERSION 4)
    # stays the same across versions, so new versions replace old ones
    set(CPACK_WIX_UPGRADE_GUID "05B4C9B0-F007-43F4-8F67-D06D49B6E20A")
    set(CPACK_WIX_PRODUCT_ICON "${Justly_SOURCE_DIR}/windows/Justly.ico")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER ".")
    set(CPACK_WIX_PATCH_FILE "${Justly_SOURCE_DIR}/windows/file_types.xml")
    # WIX takes a .txt or .rtf license
    configure_file("${Justly_SOURCE_DIR}/LICENSE.md"
        "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
    # otherwise x64
    if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(CPACK_WIX_ARCHITECTURE "arm64")
    endif()
endif()

include(CPack)