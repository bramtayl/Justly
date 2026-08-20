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

# need to include because Q_OBJECT compiles code
target_sources(Justly PRIVATE "PianoRollWidget.hpp")

if (BUILD_TESTS)
    target_sources(JustlyTests PRIVATE "PianoRollWidget.hpp")
endif()
