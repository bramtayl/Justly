target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "PianoRollColumnScene.hpp"
    "PianoRollNoteEvent.hpp"
    "PianoRollNotesScene.hpp"
    "PianoRollWidget.hpp"
    "PlayheadTransition.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "PianoRollColumnScene.cpp"
    "PianoRollNoteEvent.cpp"
    "PianoRollNotesScene.cpp"
    "PianoRollWidget.cpp"
)
