add_subdirectory(unpitched_note)
add_subdirectory(pitched_note)

target_sources(JustlyLibrary PRIVATE
    "Note.cpp"
    "Note.hpp"
)
