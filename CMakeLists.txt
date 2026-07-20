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
    # without this, the installed app can't pick up the GTK theme (native
    # widget style, icons, file dialogs) on Ubuntu -- only the system Qt
    # install has it, and the deploy script below doesn't bundle plugins
    # that aren't linked or explicitly whitelisted
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

qt_add_library(JustlyLibrary INTERFACE)
qt_add_executable(Justly)
if (BUILD_TESTS)
   qt_add_executable(JustlyTests)
   if (APPLE)
       # same reasoning as Justly's own MACOSX_BUNDLE in executable/CMakeLists.txt:
       # qt_generate_deploy_app_script()/install(TARGETS) further down need to
       # see this before they run, so it can't be deferred into
       # tests/CMakeLists.txt via add_subdirectory("tests") below. Qt's Apple
       # deploy tooling only supports bundles -- a plain (non-bundle)
       # executable target hard-errors at install time on Darwin with shared
       # Qt libs, so JustlyTests needs to be a bundle too, same as Justly.
       set_target_properties(JustlyTests PROPERTIES MACOSX_BUNDLE ON)
   endif()
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
        QGtk3ThemePlugin
    )
    set_property(TARGET Qt6::Svg APPEND PROPERTY _qt_plugins QSvgPlugin)

    # QXcbIntegrationPlugin is deployed automatically as the default platform
    # plugin, but QOffscreenIntegrationPlugin and QGtk3ThemePlugin aren't
    # defaults and need to be whitelisted explicitly. QOffscreenIntegrationPlugin
    # is only needed for ctest to run headless (QT_QPA_PLATFORM=offscreen) in
    # CI, where there's no X server for xcb to connect to; QGtk3ThemePlugin is
    # what gives the installed app native GTK styling/file dialogs on Ubuntu
    # instead of falling back to Qt's generic (non-native-looking) theme.
    qt_import_plugins(Justly INCLUDE
        Qt6::QOffscreenIntegrationPlugin
        Qt6::QGtk3ThemePlugin
    )

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

# qt_import_plugins() above is per-target and Linux-only, so JustlyTests needs
# its own call here too -- otherwise its deploy script only bundles the
# default platform plugin (xcb on Linux, cocoa on macOS), and ctest (run with
# QT_QPA_PLATFORM=offscreen on both, see workflow.yaml) fails to start with
# "no Qt platform plugin could be initialized". Windows isn't included since
# its ctest run doesn't set QT_QPA_PLATFORM. Unlike Debian/Ubuntu's split
# packages (see the find_package/_qt_plugins workarounds above), Homebrew's
# Qt already exposes Qt6::QOffscreenIntegrationPlugin transitively through
# Qt6Gui, so no extra find_package call is needed for macOS here.
if (UNIX AND BUILD_TESTS)
    qt_import_plugins(JustlyTests INCLUDE Qt6::QOffscreenIntegrationPlugin)
endif()

install(TARGETS Justly
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

if (BUILD_TESTS)
    # has to come before the platform branches below, not down by
    # add_subdirectory("tests") near the bottom of this file -- on Apple,
    # qt6_deploy_runtime_dependencies() (invoked from those branches) looks
    # for "JustlyTests.app" by name inside the install prefix rather than by
    # its build-tree path the way the Windows/Linux deploy scripts do, so the
    # bundle has to already be installed by the time that runs
    install(TARGETS JustlyTests
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
endif()

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
    # the executable, so it needs to live next to Contents/MacOS.
    # $<TARGET_BUNDLE_CONTENT_DIR:Justly> always expands to the *build tree*
    # bundle path (with_tests/Justly.app/Contents), never one rebased against
    # the install prefix -- unlike install(TARGETS ... BUNDLE DESTINATION),
    # this generator expression's result is documented as "full path to the
    # bundle content directory", i.e. already absolute, so cmake --install
    # --prefix ignores --prefix entirely for it and the files land in the
    # build tree instead of test_install/. Use a plain relative path (the
    # bundle name is just the target name + ".app", set via executable/
    # CMakeLists.txt's MACOSX_BUNDLE, with no MACOSX_BUNDLE_BUNDLE_NAME
    # override) so it's actually resolved against whatever prefix is given.
    install(DIRECTORY "${Justly_SOURCE_DIR}/share/"
        DESTINATION "Justly.app/Contents/share"
    )

    # JustlyTests is a bundle too (see the MACOSX_BUNDLE comment above), so
    # like Justly it looks for "share" inside its own Contents/ rather than
    # next to a bin/ folder. Only needed for the test install tree, so skip
    # it for release packaging to avoid duplicating the (~200MB) soundfont
    # into the shipped zip.
    if (BUILD_TESTS)
        install(DIRECTORY "${Justly_SOURCE_DIR}/share/"
            DESTINATION "JustlyTests.app/Contents/share"
        )

        # the deploy script above only walks Justly's own dependency graph,
        # which doesn't include Qt6::Test (only JustlyTests links that), so
        # JustlyTests's own load commands are left pointing at the original
        # absolute Homebrew paths -- deploying it as its own target lets
        # macdeployqt rewrite those and copy QtTest.framework alongside.
        qt_generate_deploy_app_script(
            TARGET JustlyTests
            OUTPUT_SCRIPT tests_deploy_script
            NO_TRANSLATIONS
        )
        install(SCRIPT ${tests_deploy_script})

        # macdeployqt only bundles the platform plugin matching the app's
        # default backend (cocoa) -- qt_import_plugins(JustlyTests INCLUDE
        # Qt6::QOffscreenIntegrationPlugin) above only affects static Qt
        # builds (it controls Q_IMPORT_PLUGIN codegen) and does nothing for
        # macdeployqt's own plugin selection on a dynamic build like
        # Homebrew's, so "offscreen" never makes it into the bundle on its
        # own. ctest runs with QT_QPA_PLATFORM=offscreen (see workflow.yaml
        # -- CI has no window server), which only surfaces as a failure once
        # Homebrew's Qt is uninstalled ahead of the "Test on MacOS" step,
        # since the bundle is then the only place the plugin could come
        # from. Copy it in and give it the same rpath macdeployqt gives
        # libqcocoa.dylib (two directories up from platforms/ is Contents/,
        # where Frameworks/ lives) so it can resolve @rpath/QtCore.framework
        # etc. against the frameworks the deploy script above already copied.
        # $<TARGET_BUNDLE_CONTENT_DIR:...> is documented as expanding to the
        # *full* (i.e. absolute, build-tree) bundle content directory
        # (with_tests/JustlyTests.app/Contents, which already exists once
        # JustlyTests is linked) -- never one rebased against an install
        # prefix, in install(CODE) text or anywhere else. That bit Justly's
        # own share/ install above until it was switched to a plain relative
        # path. Use the escaped \${CMAKE_INSTALL_PREFIX} instead, same as the
        # Linux branch below -- it's a literal CMake variable that only gets
        # evaluated when the generated install script itself runs, so it
        # resolves against whatever --prefix cmake --install was given.
        install(CODE "
            file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/JustlyTests.app/Contents/PlugIns/platforms\")
            file(COPY_FILE
                \"$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>\"
                \"\${CMAKE_INSTALL_PREFIX}/JustlyTests.app/Contents/PlugIns/platforms/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>\"
            )
            execute_process(COMMAND install_name_tool -add_rpath
                \"@loader_path/../../Frameworks\"
                \"\${CMAKE_INSTALL_PREFIX}/JustlyTests.app/Contents/PlugIns/platforms/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>\"
            )
        ")
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

    # X11/xcb, Wayland, GL/DRM, D-Bus/systemd/security, and PulseAudio/
    # PipeWire/ALSA client libraries are all baseline parts of any desktop
    # Ubuntu install with audio -- unlike Qt itself (bundled because
    # Ubuntu's apt Qt is too old, see the comment above) or the GTK3 theme
    # plugin's own libraries (gtk/gdk/glib/cairo/pango/atk/gdk-pixbuf,
    # bundled so the theme plugin gets a GTK build it's actually known to
    # work with), there's no version-skew reason to carry these around.
    # JACK and libdecor are deliberately NOT in this list even though GTK3/
    # FluidSynth link them the same way as the rest -- unlike PulseAudio/ALSA
    # or the core Wayland client libs, neither is part of a stock desktop
    # Ubuntu install (JACK is an opt-in package for pro-audio setups; libdecor
    # is a client-side-decoration shim pulled in by things like SDL2/SDL3/
    # xwayland, not by GNOME/KDE's own compositors or gtk itself), so both
    # stay bundled like Qt/GTK rather than assumed-present. Excluding a
    # library here also
    # prunes their own transitive deps from ever being walked in the first
    # place (e.g. libsystemd's own libblkid/libmount/libuuid dependencies
    # never get discovered once libsystemd itself is excluded) -- so this
    # list only needs the libraries actually reachable as direct NEEDED
    # entries, not every leaf. PRE_EXCLUDE_REGEXES matches against each
    # dependency's raw SONAME, not its resolved path.
    # flat prefix patterns, deliberately with no groups/alternation -- Qt's
    # deploy tool chokes ("Unmatched parentheses") on regexes combining
    # parenthesized alternation with character classes once they've round-
    # tripped through the generated deploy script
    set(system_lib_exclude_regexes
        "^libX11"
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
        "^libXRes"
        "^libXss"
        "^libXtst"
        "^libICE"
        "^libSM"
        "^libxcb"
        "^libxkbcommon"
        "^libwayland-"
        "^libEGL"
        "^libGLX"
        "^libGLdispatch"
        "^libOpenGL"
        "^libepoxy"
        "^libgbm"
        "^libdrm"
        "^libdbus-1"
        "^libsystemd"
        "^libapparmor"
        "^libselinux"
        "^libseccomp"
        "^libasound"
        "^libasyncns"
        "^libpulse"
        "^libpipewire"
    )

    qt_generate_deploy_app_script(
        TARGET Justly
        OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS
        PRE_EXCLUDE_REGEXES ${system_lib_exclude_regexes}
    )
    install(SCRIPT ${deploy_script})

    install(DIRECTORY "${Justly_SOURCE_DIR}/share/" TYPE DATA)

    if (BUILD_TESTS)
        # same reasoning as the macOS branch above: the deploy script for
        # Justly doesn't walk Qt6::Test, since only JustlyTests links it, so
        # that .so never gets copied into ../lib where JustlyTests's own
        # (Qt-CMake-default) RUNPATH looks for it.
        #
        # JustlyTests also needs the same old-style DT_RPATH treatment as
        # Justly above: its default RUNPATH resolves its own direct NEEDED
        # entries (Core/Gui/Widgets/Test/...) fine, but that's non-transitive,
        # so when one of those bundled .so's itself needs another bundled
        # library (e.g. libQt6Gui.so needing libQt6DBus.so), that lookup
        # isn't covered and falls through to the (possibly absent) system copy.
        set_target_properties(JustlyTests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        target_link_options(JustlyTests PRIVATE "LINKER:--disable-new-dtags")

        qt_generate_deploy_app_script(
            TARGET JustlyTests
            OUTPUT_SCRIPT tests_deploy_script
            NO_TRANSLATIONS
            PRE_EXCLUDE_REGEXES ${system_lib_exclude_regexes}
        )
        install(SCRIPT ${tests_deploy_script})

        # same gap as the macOS branch above: the deploy script only bundles
        # the platform plugin actually in use (xcb) -- qt_import_plugins(
        # JustlyTests INCLUDE Qt6::QOffscreenIntegrationPlugin) above doesn't
        # change that, since it's a static-Qt-build mechanism (Q_IMPORT_PLUGIN
        # codegen) and has no effect on qt6_deploy_runtime_dependencies's
        # plugin selection either. ctest runs with QT_QPA_PLATFORM=offscreen
        # (see workflow.yaml -- CI has no X server), so "offscreen" has to be
        # copied in by hand. Unlike the macOS branch, no rpath patching is
        # needed: JustlyTests's own DT_RPATH ($ORIGIN/../lib, set above) is a
        # process-wide fallback that glibc's loader also applies to a
        # plugin's own dependencies, not just JustlyTests's direct ones.
        if (CMAKE_LIBRARY_ARCHITECTURE)
            set(qt_plugins_subdir "lib/${CMAKE_LIBRARY_ARCHITECTURE}/qt6/plugins")
        else()
            set(qt_plugins_subdir "lib/qt6/plugins")
        endif()
        install(CODE "
            file(MAKE_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${qt_plugins_subdir}/platforms\")
            file(COPY_FILE
                \"$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>\"
                \"\${CMAKE_INSTALL_PREFIX}/${qt_plugins_subdir}/platforms/$<TARGET_FILE_NAME:Qt6::QOffscreenIntegrationPlugin>\"
            )
        ")
    endif()
endif()

if (BUILD_TESTS)
    add_subdirectory("tests")

    # merges into the same Contents/share/ the main share/ install above
    # already targets on Apple -- Tester.hpp looks both up the same way
    # get_share_file() does, so both need to land next to each other. Same
    # relative-path reasoning as the main share/ install above: a
    # $<TARGET_BUNDLE_CONTENT_DIR:...> DESTINATION is an absolute build-tree
    # path, so it isn't rebased against install_tests' --prefix.
    if (APPLE)
        install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/"
            DESTINATION "JustlyTests.app/Contents/share"
        )
    else()
        install(DIRECTORY "${Justly_SOURCE_DIR}/tests/share/" TYPE DATA)
    endif()

    set(test_install_folder "${Justly_SOURCE_DIR}/test_install")

    add_custom_target(install_tests ALL
        COMMAND cmake --install ${CMAKE_CURRENT_BINARY_DIR}
        --prefix ${test_install_folder}
        --config $<CONFIG>
    )

    add_dependencies(install_tests JustlyLibrary Justly JustlyTests)

    # JustlyTests is a bundle on Apple (see the MACOSX_BUNDLE comment
    # earlier), so its actual executable lives inside the bundle rather than
    # directly under bin/
    if (APPLE)
        add_test(NAME run_tests
            COMMAND "${test_install_folder}/JustlyTests.app/Contents/MacOS/JustlyTests")
    else()
        add_test(NAME run_tests COMMAND "${test_install_folder}/bin/JustlyTests")
    endif()
endif()

include(CPack)