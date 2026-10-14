target_compile_features(JustlyLibrary PUBLIC cxx_std_20)

target_link_libraries(JustlyLibrary PUBLIC
    FluidSynth::libfluidsynth
    LibXml2::LibXml2
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    Qt6::Widgets
)

if (NO_REALTIME_AUDIO)
    target_compile_definitions(JustlyLibrary PUBLIC "NO_REALTIME_AUDIO")
endif()

target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers TYPE HEADERS)

add_subdirectory("actions")
add_subdirectory("cell_editors")
add_subdirectory("cell_types")
add_subdirectory("column_numbers")
add_subdirectory("common")
add_subdirectory("io")
add_subdirectory("menus")
add_subdirectory("models")
add_subdirectory("piano_roll")
add_subdirectory("rows")
add_subdirectory("song")
add_subdirectory("sound")
add_subdirectory("widgets")

target_include_directories(JustlyLibrary PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
