#include "sound/FluidSynth.hpp"

#include <QtCore/QtAssert>
#include <fluidsynth.h>
#include <fluidsynth/synth.h>

#include "sound/FluidSettings.hpp"

FluidSynth::FluidSynth(FluidSettings &settings)
    : internal_pointer(new_fluid_synth(settings.internal_pointer)) {
  Q_ASSERT(internal_pointer != nullptr);
}

FluidSynth::~FluidSynth() { delete_fluid_synth(internal_pointer); }
