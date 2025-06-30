#pragma once

#include "other/helpers.hpp"
#include "sound/FluidSettings.hpp"

struct FluidSynth {
  fluid_synth_t *const internal_pointer;

  explicit FluidSynth(FluidSettings &settings)
      : internal_pointer(new_fluid_synth(settings.internal_pointer)) {}

  NO_MOVE_COPY(FluidSynth)

  ~FluidSynth() { delete_fluid_synth(internal_pointer); }
};

