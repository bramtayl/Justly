# 3.25 for the LINUX variable
cmake_minimum_required(VERSION 3.25)

# on windows, add a path to Qt, e.g.
# list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

# must be set before project(), otherwise CMake defaults to build version
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

project(Justly VERSION 1.2.0 LANGUAGES CXX)

# the soundfont is too big to commit to git, so download it if it's missing
# (its license, MS_Basic_License.md, is committed)
set(soundfont_file "${Justly_SOURCE_DIR}/share/MS_Basic.sf3")
if (NOT EXISTS "${soundfont_file}")
    message(STATUS "Downloading MS_Basic.sf3")
    # download next to the build first, so a failed download doesn't leave
    # a partial file behind that the EXISTS check above would accept
    file(DOWNLOAD
        "https://github.com/musescore/MuseScore/raw/v4.7.4/share/sound/MS%20Basic.sf3"
        "${CMAKE_BINARY_DIR}/MS_Basic.sf3"
        EXPECTED_HASH SHA256=5ea2375e8bd7d8e71def1036978c1621e85b66934169b6a2744b27b9b3c2d99c
    )
    # not file(RENAME), which fails if the build dir is on another filesystem
    file(COPY_FILE "${CMAKE_BINARY_DIR}/MS_Basic.sf3" "${soundfont_file}")
    file(REMOVE "${CMAKE_BINARY_DIR}/MS_Basic.sf3")
endif()

include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

if (LINUX)
    # rpath fixes is broken in qt_generate_deploy_app_script
    # because ubuntu installed plugins don't have an rpath
    # need to set before find_package(Qt6)
    set(QT_DEPLOY_FORCE_ADJUST_RPATHS OFF)
endif()

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg Widgets)
if (BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

if (LINUX)
    # in Debian, these are separate packages in Qt6Gui_DIR
    list(APPEND CMAKE_PREFIX_PATH "${Qt6Gui_DIR}")
    # to look native
    find_package(Qt6QGtk3ThemePlugin REQUIRED PATHS "${Qt6Gui_DIR}")
    if (BUILD_TESTS)
        # needed for ctest to run offscreen
        find_package(Qt6QOffscreenIntegrationPlugin REQUIRED PATHS "${Qt6Gui_DIR}")
    endif()
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

if (LINUX)
    # qt_import_plugins won't work with the offscreen plugin
    qt_import_plugins(Justly INCLUDE Qt6::QGtk3ThemePlugin)

    # qt will deploy system libraries by default on linux
    # so exclude them manually
    set(system_lib_exclude_regexes
        "^ld-linux"
        "^libEGL"
        "^libGLX"
        "^libGLdispatch"
        "^libICE"
        "^libOpenGL"
        "^libSM"
        # all the X11 client libraries (libX11, libXext, libXrandr, ...)
        "^libX"
        "^libapparmor"
        "^libasound"
        "^libasyncns"
        "^libbz2"
        "^libc[.]"
        "^libcrypto[.]"
        "^libdb-"
        "^libdbus-1"
        "^libdrm"
        "^libepoxy"
        "^libexpat"
        "^libffi"
        "^libgbm"
        "^libgcc_s"
        "^libgmp"
        "^libgnutls"
        "^libgssapi_krb5"
        "^libhogweed"
        "^libidn2"
        "^libm[.]"
        "^libmount"
        "^libmvec"
        "^libnettle"
        "^libnghttp2"
        "^libpcre2-8"
        "^libpipewire"
        "^libpsl"
        "^libpulse"
        "^libreadline"
        "^libseccomp"
        "^libselinux"
        "^libssl[.]"
        "^libstdc[+][+]"
        "^libsystemd"
        "^libudev"
        "^libwayland-"
        "^libxcb"
        "^libxkbcommon"
        "^libxml2"
        "^libz[.]"
        "^libzstd"
    )
else()
    set(system_lib_exclude_regexes "")
endif()

# install the share folder, and also put it into the build dir so the target works there too
function(justly_install_share share_folder bundle_target)
    if (APPLE)
        set(share_sub_folder "Resources")
        set(install_sub_folder "${bundle_target}.app/Contents/")
    else()
        set(share_sub_folder "share")
        set(install_sub_folder "")
    endif()

    install(DIRECTORY "${share_folder}" DESTINATION "${install_sub_folder}${share_sub_folder}")

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

    if (LINUX)
        # need to set rpath on linux
        set_target_properties(${app_target} PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        # if plugins inherit justly's rpath, since they are in a different location, the rpath won't work
        # so disable rpath inheritance
        target_link_options(${app_target} PRIVATE "LINKER:--disable-new-dtags")
    endif()

    qt_generate_deploy_app_script(
        TARGET ${app_target}
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        PRE_EXCLUDE_REGEXES ${system_lib_exclude_regexes}
    )
    install(SCRIPT ${deploy_script})

    justly_install_share("${Justly_SOURCE_DIR}/share/" ${app_target})
endforeach()

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
    # qt_import_plugins won't work for platform plugins
    # so the offscreen plugin needs to be deployed manually
    if (APPLE)
        set(platform_plugin_subfolder "JustlyTests.app/Contents/PlugIns/platforms")
    elseif (WIN32)
        set(platform_plugin_subfolder "bin/platforms")
    else()
        if (CMAKE_LIBRARY_ARCHITECTURE)
            set(architecture_folder "${CMAKE_LIBRARY_ARCHITECTURE}/")
        else()
            set(architecture_folder "")
        endif()
        set(platform_plugin_subfolder "lib/${architecture_folder}qt6/plugins/platforms")
    endif()

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

include(CPack)