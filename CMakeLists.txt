cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

project(Justly VERSION 1.0.0 LANGUAGES CXX)

# download https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General/MuseScore_General.sf2
# and put it into the share folder
# too big to commit to git

include(CTest)
include(InstallRequiredSystemLibraries)

option(BUILD_TESTS "Build tests" OFF)
option(NO_REALTIME_AUDIO "Do not start realtime audio" OFF)
option(INCLUDE_WHAT_YOU_USE "Run include-what-you-use" OFF)
option(TRACK_COVERAGE "Track coverage" OFF)

if (UNIX AND NOT APPLE)
    # need to set before find_package(Qt6Core)
    set(QT_DEPLOY_FORCE_ADJUST_RPATHS OFF)
endif()

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Widgets REQUIRED)

if (UNIX AND NOT APPLE)
    # in Debian/Ubuntu, plugins have separate packages
    find_package(Qt6QSvgPlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" "${Qt6Svg_DIR}" NO_DEFAULT_PATH)
    find_package(Qt6QXcbIntegrationPlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" NO_DEFAULT_PATH)
    # needed for ctest to run offsecreen
    find_package(Qt6QOffscreenIntegrationPlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" NO_DEFAULT_PATH)
    # GTK theme on Ubuntu
    find_package(Qt6QGtk3ThemePlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" NO_DEFAULT_PATH)
endif()

qt_standard_project_setup()

if (INCLUDE_WHAT_YOU_USE)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE 
        include-what-you-use
        -Xiwyu
        --mapping_file=${Justly_SOURCE_DIR}/map.imp
    )
endif()

if (MSVC)
    add_compile_options("/W4")
else()
    add_compile_options("-Wall" "-Wextra" "-Wpedantic" "$<$<CONFIG:Debug>:-Og>")
endif()

if (TRACK_COVERAGE)
    add_compile_options("--coverage")
    add_link_options("--coverage")
endif()

# mirror the installed layout (bin/ next to share/, or Contents/MacOS next to
# Contents/Resources) inside the build tree too, so Justly/JustlyTests can
# find share/ (see get_share_folder() in helpers.hpp) straight out of the
# build directory, without an install step or an environment variable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

qt_add_library(JustlyLibrary INTERFACE)
qt_add_executable(Justly)
if (BUILD_TESTS)
   qt_add_executable(JustlyTests)
   if (APPLE)
       # need to call before qt_generate_deploy_app_script()
       set_target_properties(JustlyTests PROPERTIES MACOSX_BUNDLE ON)
   endif()
endif()

# the set of app targets that get installed/deployed the same way; used to
# share install/deploy logic between Justly and JustlyTests below
set(app_targets Justly)
if (BUILD_TESTS)
    list(APPEND app_targets JustlyTests)
endif()

add_subdirectory("library")
add_subdirectory("executable")

# executable/CMakeLists.txt sets MACOSX_BUNDLE ON for Justly on Apple
# platforms (and JustlyTests mirrors it above); test the target's actual
# bundle-ness rather than assuming any particular OS builds a bundle
get_target_property(is_bundle Justly MACOSX_BUNDLE)

if (UNIX AND NOT APPLE)
    # need since plugins have separate packages
    set_property(TARGET Qt6::Gui APPEND PROPERTY _qt_plugins
        QXcbIntegrationPlugin
        QOffscreenIntegrationPlugin
        QGtk3ThemePlugin
    )
    set_property(TARGET Qt6::Svg APPEND PROPERTY _qt_plugins QSvgPlugin)

    # need to import non-default plugins
    qt_import_plugins(Justly INCLUDE
        Qt6::QGtk3ThemePlugin
    )
endif()

# need to call before qt_generate_deploy_app_script
foreach(app_target ${app_targets})
    install(TARGETS ${app_target}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
endforeach()

# regexes for Qt's deploy script to skip: system libraries already provided
# by the OS on Linux; empty elsewhere so the same deploy call below works on
# every platform
set(system_lib_exclude_regexes "")
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
endif()

if (UNIX AND NOT APPLE)
    # need to set rpath on linux
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    # force plugins to use Justly's rpath
    target_link_options(Justly PRIVATE "LINKER:--disable-new-dtags")

    if (BUILD_TESTS)
        set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        target_link_options(JustlyTests PRIVATE "LINKER:--disable-new-dtags")
    endif()
endif()

foreach(app_target ${app_targets})
    # PRE_EXCLUDE_REGEXES requires a fairly new version of Qt
    qt_generate_deploy_app_script(
        TARGET ${app_target}
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        PRE_EXCLUDE_REGEXES ${system_lib_exclude_regexes}
    )
    install(SCRIPT ${deploy_script})
endforeach()

if (WIN32)
    # windeployqt only deploys qt dependencies
    # need to install other dependencies manually
    foreach(app_target ${app_targets})
        install(TARGETS ${app_target} RUNTIME_DEPENDENCY_SET vcpkg_deps)
    endforeach()

    install(RUNTIME_DEPENDENCY_SET vcpkg_deps
        DIRECTORIES "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin"
        PRE_INCLUDE_REGEXES
            "^glib-2\\.0-0\\.dll$"
            "^iconv-2\\.dll$"
            "^intl-8\\.dll$"
            "^libfluidsynth-3\\.dll$"
            "^libxml2\\.dll$"
            "^pcre2-16d?\\.dll$"
            "^pcre2-8d?\\.dll$"
            "^zlibd?1\\.dll$"
        PRE_EXCLUDE_REGEXES ".*"
        RUNTIME DESTINATION bin
    )
endif()

if (is_bundle)
    foreach(app_target ${app_targets})
        install(DIRECTORY "${Justly_SOURCE_DIR}/share/"
            DESTINATION "${app_target}.app/Contents/Resources"
        )
    endforeach()
else()
    install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)
endif()

foreach(app_target ${app_targets})
    if (is_bundle)
        set(build_share_dest "$<TARGET_FILE_DIR:${app_target}>/../Resources")
    else()
        set(build_share_dest "$<TARGET_FILE_DIR:${app_target}>/../share")
    endif()
    add_custom_command(TARGET ${app_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${Justly_SOURCE_DIR}/share" "${build_share_dest}"
    )
endforeach()

if (BUILD_TESTS AND NOT WIN32)
    # qt deploy includes plugins added via qt_import_plugins
    # but ignores non-default PLATFORM plugins
    # need to deploy QOffscreenIntegrationPlugin manually
    if (is_bundle)
        set(offscreen_plugin_dir "JustlyTests.app/Contents/PlugIns/platforms")
    else()
        set(qt_lib_arch_dir "")
        if (CMAKE_LIBRARY_ARCHITECTURE)
            set(qt_lib_arch_dir "${CMAKE_LIBRARY_ARCHITECTURE}/")
        endif()
        set(offscreen_plugin_dir "lib/${qt_lib_arch_dir}qt6/plugins/platforms")
    endif()

    install(CODE "
        file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${offscreen_plugin_dir}\")
        file(COPY_FILE
            \"$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>\"
            \"\${CMAKE_INSTALL_PREFIX}/${offscreen_plugin_dir}/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>\"
        )
    ")

    if (is_bundle)
        # bundle plugins load relative to the bundle's Frameworks directory
        install(CODE "
            execute_process(COMMAND install_name_tool -add_rpath
                \"@loader_path/../../Frameworks\"
                \"\${CMAKE_INSTALL_PREFIX}/${offscreen_plugin_dir}/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>\"
            )
        ")
    endif()
endif()

if (BUILD_TESTS)
    add_subdirectory("tests")

    if (is_bundle)
        install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/"
            DESTINATION "JustlyTests.app/Contents/Resources"
        )
        set(build_tests_share_dest "$<TARGET_FILE_DIR:JustlyTests>/../Resources")
    else()
        install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/" TYPE DATA)
        set(build_tests_share_dest "$<TARGET_FILE_DIR:JustlyTests>/../share")
    endif()

    add_custom_command(TARGET JustlyTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${Justly_SOURCE_DIR}/tests/share" "${build_tests_share_dest}"
    )

    set(test_install_folder "${Justly_SOURCE_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    # QTest's plain-text logger (implicit default, or "-o -,txt" for stdout)
    # never reaches ctest's captured output on Windows CI -- Start/Totals/
    # Finished lines never appear even though the test itself runs and
    # passes. Only writing to a real file works, so force that and have the
    # workflow cat the file afterwards to surface it in the CI log.
    if (is_bundle)
        add_test(NAME run_tests
            COMMAND "${test_install_folder}/JustlyTests.app/Contents/MacOS/JustlyTests" -o test_output.txt,txt)
        # separate from run_tests: launches via LaunchServices (as Finder
        # would) to catch bundle-structure problems that direct-binary
        # invocation above wouldn't (e.g. bad Info.plist); open's own exit
        # code only reflects whether the launch succeeded, not the app's
        # exit status, so this can't replace run_tests as a pass/fail check
        add_test(NAME bundle_opens
            COMMAND open -W -a "${test_install_folder}/JustlyTests.app")
    else()
        add_test(NAME run_tests COMMAND "${test_install_folder}/bin/JustlyTests" -o test_output.txt,txt)
    endif()
endif()

include(CPack)