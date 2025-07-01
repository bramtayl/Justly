target_sources(Justly PRIVATE "Justly.cpp")

if (WIN32)
    set_target_properties(Justly PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_link_libraries(Justly PRIVATE JustlyLibrary)
