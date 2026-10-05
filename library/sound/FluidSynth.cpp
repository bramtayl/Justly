#include "sound/FluidSynth.hpp"

#include "sound/FluidSettings.hpp"

FluidSynth::FluidSynth(FluidSettings& settings)
    : CHandle(new_fluid_synth(settings.internal_pointer)) {
  Q_ASSERT(internal_pointer != nullptr);
}
