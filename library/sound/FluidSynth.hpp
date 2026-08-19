#pragma once

#include <fluidsynth/types.h>

#include "other/helpers.hpp"

struct FluidSettings;

struct FluidSynth {
  fluid_synth_t *const internal_pointer;

  explicit FluidSynth(FluidSettings &settings);

  NO_MOVE_COPY(FluidSynth)

  ~FluidSynth();
};
