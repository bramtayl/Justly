
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "Cells.hpp"
    "PianoRollNoteEvent.hpp"
    "Song.hpp"
    "TableSelection.hpp"
    "helpers.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "PianoRollNoteEvent.cpp"
    "Song.cpp"
    "helpers.cpp"
)
