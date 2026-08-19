target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES
    "PianoRollAxisScene.hpp"
    "PianoRollLegendScene.hpp"
    "PianoRollNotesScene.hpp"
    "PianoRollWidget.hpp"
    "PlayheadTransition.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "PianoRollAxisScene.cpp"
    "PianoRollLegendScene.cpp"
    "PianoRollNotesScene.cpp"
    "PianoRollWidget.cpp"
)
