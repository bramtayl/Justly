target_sources(JustlyLibrary PUBLIC FILE_SET justly_headers FILES 
    "FluidDriver.hpp"
    "FluidEvent.hpp"
    "FluidSequencer.hpp"
    "FluidSettings.hpp"
    "FluidSynth.hpp"
    "PlayState.hpp"
    "Player.hpp"
)

target_sources(JustlyLibrary PRIVATE
    "FluidDriver.cpp"
    "FluidEvent.cpp"
    "FluidSequencer.cpp"
    "FluidSettings.cpp"
    "FluidSynth.cpp"
    "Player.cpp"
)
