if(APPLE)
elseif(WIN32)
else()
  find_library(QtPlugins_Wayland_LIBRARY NAMES "qwayland-generic" HINTS
    "/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms"
  )
  find_library(QtPlugins_WaylandShell_LIBRARY NAMES "xdg-shell" HINTS
    "/usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-shell-integration"
  )
  find_library(QtPlugins_WaylandGraphics_LIBRARY NAMES "qt-plugin-wayland-egl" HINTS
    "/usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-graphics-integration-client"
  )
  find_library(QtPlugins_WaylandDecoration_LIBRARY NAMES "bradient" HINTS
    "/usr/lib/x86_64-linux-gnu/qt6/plugins/wayland-decoration-client"
  )
  find_library(QtPlugins_GTKStyle_LIBRARY NAMES "qgtk3" HINTS
    "/usr/lib/x86_64-linux-gnu/qt6/plugins/platformthemes"
  )

  find_package_handle_standard_args(QtPlugins
    QtPlugins_Wayland_LIBRARY
    QtPlugins_WaylandShell_LIBRARY
    QtPlugins_WaylandGraphics_LIBRARY
    QtPlugins_WaylandDecoration_LIBRARY
    QtPlugins_GTKStyle_LIBRARY
  )

  if(QtPlugins_FOUND AND NOT TARGET QtPlugins::Wayland)
    add_library(QtPlugins::Wayland SHARED IMPORTED)
    set_target_properties(QtPlugins::Wayland PROPERTIES
      IMPORTED_LOCATION "${QtPlugins_Wayland_LIBRARY}"
    )
  endif()

  if(QtPlugins_FOUND AND NOT TARGET QtPlugins::WaylandShell)
    add_library(QtPlugins::WaylandShell SHARED IMPORTED)
    set_target_properties(QtPlugins::WaylandShell PROPERTIES
      IMPORTED_LOCATION "${QtPlugins_WaylandShell_LIBRARY}"
    )
  endif()

  if(QtPlugins_FOUND AND NOT TARGET QtPlugins::WaylandGraphics)
    add_library(QtPlugins::WaylandGraphics SHARED IMPORTED)
    set_target_properties(QtPlugins::WaylandGraphics PROPERTIES
      IMPORTED_LOCATION "${QtPlugins_WaylandGraphics_LIBRARY}"
    )
  endif()

  if(QtPlugins_FOUND AND NOT TARGET QtPlugins::WaylandDecoration)
    add_library(QtPlugins::WaylandDecoration SHARED IMPORTED)
    set_target_properties(QtPlugins::WaylandDecoration PROPERTIES
      IMPORTED_LOCATION "${QtPlugins_WaylandDecoration_LIBRARY}"
    )
  endif()

  if(QtPlugins_FOUND AND NOT TARGET QtPlugins::GTKStyle)
    add_library(QtPlugins::GTKStyle SHARED IMPORTED)
    set_target_properties(QtPlugins::GTKStyle PROPERTIES
      IMPORTED_LOCATION "${QtPlugins_GTKStyle_LIBRARY}"
    )
  endif()
endif()