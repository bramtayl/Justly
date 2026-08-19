target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "Chord.hpp"
    "Note.hpp"
    "PitchedNote.hpp"
    "PitchedVoice.hpp"
    "Row.hpp"
    "RowType.hpp"
    "UnpitchedNote.hpp"
    "UnpitchedVoice.hpp"
    "Voice.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "Chord.cpp"
    "PitchedNote.cpp"
    "PitchedVoice.cpp"
    "Row.cpp"
    "UnpitchedNote.cpp"
    "UnpitchedVoice.cpp"
)
