
target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "Cells.hpp"
    "MidiTrackEvent.hpp"
    "PianoRollNoteEvent.hpp"
    "Song.hpp"
    "helpers.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "MidiTrackEvent.cpp"
    "PianoRollNoteEvent.cpp"
    "Song.cpp"
    "helpers.cpp"
)
