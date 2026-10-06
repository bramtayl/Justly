target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "FluidDriver.hpp"
    "FluidEvent.hpp"
    "FluidSequencer.hpp"
    "FluidSettings.hpp"
    "FluidSynth.hpp"
    "PlayState.hpp"
    "Playback.hpp"
    "Player.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "FluidEvent.cpp"
    "FluidSequencer.cpp"
    "FluidSettings.cpp"
    "FluidSynth.cpp"
    "Playback.cpp"
    "Player.cpp"
)
