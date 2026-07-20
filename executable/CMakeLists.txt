target_sources(Justly PRIVATE "Justly.cpp")

if (WIN32)
    set_target_properties(Justly PROPERTIES WIN32_EXECUTABLE ON)
elseif (APPLE)
    # install(TARGETS) in the top-level CMakeLists.txt decides whether to
    # install Justly as a plain RUNTIME executable or as a BUNDLE using
    # MACOSX_BUNDLE's value at the point that command runs, not deferred to
    # generate time, so this has to be set before that call, not alongside
    # the rest of the macOS deployment logic
    set_target_properties(Justly PROPERTIES MACOSX_BUNDLE ON)
endif()

target_link_libraries(Justly PRIVATE JustlyLibrary)
