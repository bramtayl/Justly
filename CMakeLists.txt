cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt, e.g.
# list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

# must be set before project(), otherwise CMake defaults to build version
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

project(Justly VERSION 1.1.4 LANGUAGES CXX)

# download https://github.com/musescore/MuseScore/raw/v4.7.4/share/sound/MS%20Basic.sf3
# and https://github.com/musescore/MuseScore/blob/v4.7.4/share/sound/MS%20Basic_License.md
# put it into the share folder as MS_Basic.sf3 abd MS_Basic_License.sf3
# too big to commit to git

include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

if (UNIX AND NOT APPLE)
    # rpath fixes is broken in qt_generate_deploy_app_script
    # because ubuntu installed plugins don't have an rpath
    # need to set before find_package(Qt6Core)
    set(QT_DEPLOY_FORCE_ADJUST_RPATHS OFF)
endif()

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Widgets REQUIRED)

if (UNIX AND NOT APPLE)
    # in Debian, these are separate packages in Qt6Gui_DIR
    list(APPEND CMAKE_PREFIX_PATH "${Qt6Gui_DIR}")
    # needed for ctest to run offsecreen
    find_package(Qt6QOffscreenIntegrationPlugin REQUIRED PATHS "${Qt6Gui_DIR}")
    # to look native
    find_package(Qt6QGtk3ThemePlugin REQUIRED PATHS "${Qt6Gui_DIR}")
endif()

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    # headers without  mathching .cpp files won't get checked by default
    # so specify all header dirs
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
        include-what-you-use
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
        -Xiwyu
        --check_also=${Justly_SOURCE_DIR}/library/*
        -Xiwyu
        --check_also=${Justly_SOURCE_DIR}/library/*/*
        -Xiwyu
        --check_also=${Justly_SOURCE_DIR}/library/*/*/*
        -Xiwyu
        --check_also=${Justly_SOURCE_DIR}/executable/*
        -Xiwyu
        --check_also=${Justly_SOURCE_DIR}/tests/*
    )
endif()

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall" "-Wextra" "-Wpedantic")
    if (TRACK_COVERAGE)
        add_compile_options("-Og")
    else()
        add_compile_options("$<$<CONFIG:Debug>:-Og>")
    endif()
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
if (BUILD_TESTS)
   qt_add_executable(JustlyTests)
   if (APPLE)
       # need to call before qt_generate_deploy_app_script()
       set_target_properties(JustlyTests PROPERTIES MACOSX_BUNDLE ON)
   endif()
endif()

set(app_targets Justly)
if (BUILD_TESTS)
    list(APPEND app_targets JustlyTests)
endif()

add_subdirectory("library")
add_subdirectory("executable")

if (UNIX AND NOT APPLE)
    # qt_import_plugins won't work with the offscreen plugin
    qt_import_plugins(Justly INCLUDE Qt6::QGtk3ThemePlugin)
endif()

# qt will deploy system libraries by default on linux
# so exclude them manually
if (UNIX AND NOT APPLE)
    set(system_lib_exclude_regexes
        "^ld-linux"
        "^libEGL"
        "^libGLX"
        "^libGLdispatch"
        "^libICE"
        "^libOpenGL"
        "^libSM"
        "^libX11"
        "^libXRes"
        "^libXau"
        "^libXcomposite"
        "^libXcursor"
        "^libXdamage"
        "^libXdmcp"
        "^libXext"
        "^libXfixes"
        "^libXi"
        "^libXinerama"
        "^libXrandr"
        "^libXrender"
        "^libXss"
        "^libXtst"
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

    if (UNIX AND NOT APPLE)
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

    if (WIN32)
        # install manually to get runtime dependencies
        install(TARGETS ${app_target} RUNTIME_DEPENDENCY_SET dependencies)
    endif()
endforeach()

if (WIN32)
    # normalize path separators
    cmake_policy(SET CMP0207 NEW)
    
    install(RUNTIME_DEPENDENCY_SET dependencies
        # needs path to vcpkg dependencies
        DIRECTORIES "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin"
        # skip system libraries
        PRE_EXCLUDE_REGEXES
            "^api-ms-"
            "^ext-ms-"
            "^qt6"
            "^hvsifiletrust\\.dll$"
            "^wtdccm\\.dll$"
        POST_EXCLUDE_REGEXES
            ".*/System32/.*\\.dll"
        RUNTIME DESTINATION bin
    )
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
endif()

if (BUILD_TESTS)
    add_subdirectory("tests")

    justly_install_share("${Justly_SOURCE_DIR}/tests/share/" JustlyTests)

    set(test_install_folder "${Justly_SOURCE_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    if (APPLE)
        set(run_tests_subfolder "JustlyTests.app/Contents/MacOS")
        set(run_tests_args "")
    else()
        set(run_tests_subfolder "bin")
        if (WIN32)
            # test output doesn't get piped to stdout on windows
            set(run_tests_args -o test_output.txt,txt)
        else()
            set(run_tests_args "")
        endif()
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