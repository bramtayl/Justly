add_subdirectory(program)
add_subdirectory(percussion_instrument)

target_sources(JustlyLibrary PRIVATE
    "Named.cpp"
    "Named.hpp"
    "NamedEditor.hpp"
)