target_compile_features(JustlyLibrary INTERFACE cxx_std_20)

target_link_libraries(JustlyLibrary INTERFACE
    FluidSynth::libfluidsynth
    LibXml2::LibXml2
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    Qt6::Widgets
)

add_compile_definitions(JustlyLibrary INTERFACE "$<$<CONFIG:Release>:QT_NO_DEBUG=1>")
if (NO_REALTIME_AUDIO)
    target_compile_definitions(JustlyLibrary INTERFACE "NO_REALTIME_AUDIO")
endif()

target_sources(JustlyLibrary INTERFACE FILE_SET justly_headers TYPE HEADERS)

add_subdirectory("actions")
add_subdirectory("cell_editors")
add_subdirectory("cell_types")
add_subdirectory("column_numbers")
add_subdirectory("iterators")
add_subdirectory("menus")
add_subdirectory("models")
add_subdirectory("musicxml")
add_subdirectory("other")
add_subdirectory("sound")
add_subdirectory("widgets")
add_subdirectory("xml")

target_include_directories(JustlyLibrary INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
