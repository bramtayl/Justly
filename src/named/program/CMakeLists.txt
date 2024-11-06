add_subdirectory(instrument)
add_subdirectory(percussion_set)

target_sources(JustlyLibrary PRIVATE
    "Program.cpp"
    "Program.hpp"
)