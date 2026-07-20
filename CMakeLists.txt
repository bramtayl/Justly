cmake_minimum_required(VERSION 3.24)

# on windows, add a path to Qt
list(APPEND CMAKE_PREFIX_PATH C:/Users/brand/6.5.1/msvc2019_64)

project(Justly VERSION 0.8.1 LANGUAGES CXX)

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
    # find_package(Qt6Core) below is what bakes in whether the deploy script
    # patches each deployed plugin's own RPATH, so this has to be set before
    # that call -- see the Linux install() branch further down for why it's
    # disabled.
    set(QT_DEPLOY_FORCE_ADJUST_RPATHS OFF)
endif()

find_package(FluidSynth REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6Test REQUIRED)
find_package(Qt6Widgets REQUIRED)
# find_package(SDL3 REQUIRED)

if (UNIX AND NOT APPLE)
    # unlike the vcpkg Qt used on Windows, the Debian/Ubuntu Qt packages
    # split each plugin into its own CMake package config, alongside
    # Qt6Gui's (or, for the svg plugin, sometimes Qt6Svg's) own instead of
    # being exposed transitively through it, so it has to be found explicitly
    find_package(Qt6QSvgPlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" "${Qt6Svg_DIR}" NO_DEFAULT_PATH)
    find_package(Qt6QXcbIntegrationPlugin REQUIRED
        PATHS "${Qt6Gui_DIR}" NO_DEFAULT_PATH)
    # needed for ctest to run headless (QT_QPA_PLATFORM=offscreen) in CI,
    # where there's no X server for the xcb plugin to connect to
    find_package(Qt6QOffscreenIntegrationPlugin REQUIRED
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

qt_add_library(JustlyLibrary INTERFACE)
qt_add_executable(Justly)
if (BUILD_TESTS)
   qt_add_executable(JustlyTests)
endif()

add_subdirectory("library")
add_subdirectory("executable")

if (UNIX AND NOT APPLE)
    # unlike the vcpkg Qt used on Windows, the Debian/Ubuntu Qt packages
    # split each plugin into its own CMake package config instead of the
    # owning module's config registering them, so qt_generate_deploy_app_script's
    # automatic plugin detection below -- which walks each dependency's
    # _qt_plugins property -- finds nothing unless that property is set here
    set_property(TARGET Qt6::Gui APPEND PROPERTY _qt_plugins
        QXcbIntegrationPlugin
        QOffscreenIntegrationPlugin
    )
    set_property(TARGET Qt6::Svg APPEND PROPERTY _qt_plugins QSvgPlugin)

    # QXcbIntegrationPlugin is deployed automatically as the default platform
    # plugin, but QOffscreenIntegrationPlugin isn't a default and needs to be
    # whitelisted explicitly -- it's only needed for ctest to run headless
    # (QT_QPA_PLATFORM=offscreen) in CI, where there's no X server for xcb to
    # connect to
    qt_import_plugins(Justly INCLUDE Qt6::QOffscreenIntegrationPlugin)

    # by default the deploy script also patches each deployed plugin's own
    # RPATH so it can find its dependencies -- but most of Debian/Ubuntu's
    # plugin .so files ship without an existing RPATH for it to rewrite,
    # which is a hard configure-time error (CMake's file(RPATH_SET) can only
    # edit an existing entry, not add one from scratch). Disabling it here
    # (see the QT_DEPLOY_FORCE_ADJUST_RPATHS set above find_package(Qt6Core))
    # instead leans on Justly's own old-style RPATH set in the install()
    # branch below, which acts as a process-wide fallback covering the
    # plugins too.
endif()

install(TARGETS Justly
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if (WIN32)
    set(MAYBE_D "$<$<CONFIG:Debug>:d>")

    set(VCPKG_BIN_FOLDER "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}$<$<CONFIG:Debug>:/debug>/bin")

    # unlike the Linux packages, this Qt comes from the official aqt-distributed
    # binaries (jurplel/install-qt-action in the workflow), not vcpkg, so
    # windeployqt -- and qt_generate_deploy_app_script's wrapper for it -- can
    # be trusted to deploy Qt's own DLLs, plugins, and qt.conf without the
    # _qt_plugins workarounds the Linux branch above needs.
    qt_generate_deploy_app_script(
        TARGET Justly
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
    )
    install(SCRIPT ${deploy_script})

    # windeployqt only knows about Qt's own DLLs, so the vcpkg-provided
    # third-party ones still need listing manually, same as before. Qt6::Test
    # is here for the same reason: it's only linked by JustlyTests, not by
    # Justly, so scanning Justly.exe (above) won't find it either.
    install(FILES
        "${VCPKG_BIN_FOLDER}/glib-2.0-0.dll"
        "${VCPKG_BIN_FOLDER}/iconv-2.dll"
        "${VCPKG_BIN_FOLDER}/intl-8.dll"
        "$<TARGET_FILE:FluidSynth::libfluidsynth>"
        "$<TARGET_FILE:LibXml2::LibXml2>"
        "${VCPKG_BIN_FOLDER}/pcre2-16${MAYBE_D}.dll"
        "${VCPKG_BIN_FOLDER}/pcre2-8${MAYBE_D}.dll"
        "$<TARGET_FILE:Qt6::Test>"
        "${VCPKG_BIN_FOLDER}/zlib${MAYBE_D}1.dll"
        TYPE BIN
    )

    install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)
elseif (APPLE)
    # There's no CMake-native way to fix up a dylib's install name/rpath, so
    # install(RUNTIME_DEPENDENCIES) can't produce a relocatable bundle on its
    # own here. macdeployqt already walks the whole dependency graph -- Qt
    # frameworks, Qt plugins, and third-party dylibs like fluidsynth/libxml2
    # -- and rewrites install names for all of them, so let it do the whole
    # job via CMake's wrapper for it instead of re-implementing that.
    # (MACOSX_BUNDLE itself is set in executable/CMakeLists.txt, since it
    # has to be set before the install(TARGETS Justly ...) call above.)
    qt_generate_deploy_app_script(
        TARGET Justly
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
    )
    install(SCRIPT ${deploy_script})

    # get_share_file() looks for a "share" folder next to the folder holding
    # the executable, so it needs to live next to Contents/MacOS
    install(DIRECTORY "${Justly_SOURCE_DIR}/share/"
        DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:Justly>/share"
    )

    # JustlyTests isn't a bundle, so it looks for "share" next to its own
    # bin/ folder rather than inside a bundle's Contents/ -- install a plain
    # copy there too, same as the Windows/Linux branches below. Only needed
    # for the test install tree, so skip it for release packaging to avoid
    # duplicating the (~200MB) soundfont into the shipped zip.
    if (BUILD_TESTS)
        install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)
    endif()
else()
    # Qt 6.5+ ships qt_generate_deploy_app_script() support for Linux,
    # walking the runtime library closure the same way the manual
    # file(GET_RUNTIME_DEPENDENCIES) approach this replaces did, but with
    # qt.conf/plugin handling built in -- same idea as the macOS branch
    # above. Needs a newer Qt than Ubuntu noble's (24.04) apt packages ship
    # (6.4.2), so this targets the ubuntu-26.04 runner (Qt 6.10.2) in the
    # workflow instead of ubuntu-latest.
    #
    # Most of Debian/Ubuntu's own .so files (e.g. libz, libxml2, libvorbis,
    # and the Qt plugins themselves) are built without any RPATH of their
    # own, so there's no per-file RPATH for the deploy script to patch onto
    # the copies. Instead, link Justly itself with the old-style DT_RPATH
    # (--disable-new-dtags), which glibc's loader treats as a process-wide
    # fallback search path for every transitively loaded library -- not just
    # Justly's own dependencies, but the plugins' too. Deployed libraries
    # land in ../lib relative to the executable in bin/.
    set_target_properties(Justly PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    target_link_options(Justly PRIVATE "LINKER:--disable-new-dtags")

    qt_generate_deploy_app_script(
        TARGET Justly
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
    )
    install(SCRIPT ${deploy_script})

    install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)
endif()

if (BUILD_TESTS)
    add_subdirectory("tests")

    install(TARGETS JustlyTests)

    install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/" TYPE DATA)

    set(test_install_folder "${Justly_SOURCE_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    add_test(NAME run_tests COMMAND "${test_install_folder}/bin/JustlyTests")
endif()

include(CPack)