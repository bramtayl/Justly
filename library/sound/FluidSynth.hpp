#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidSettings;

struct FluidSynth : CHandle<fluid_synth_t, delete_fluid_synth> {
  explicit FluidSynth(FluidSettings& settings);
};
