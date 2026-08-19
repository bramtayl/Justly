#pragma once

#include <fluidsynth/types.h>

#include "other/helpers.hpp"

struct FluidSettings {
  fluid_settings_t *const internal_pointer;

  // midi_channels/cpu_cores/audio_driver must be set (when wanted) before
  // the settings are handed to new_fluid_synth() -- left at their defaults,
  // no playback config is applied, matching plain fluidsynth defaults;
  // audio_driver is null on non-Linux platforms, where fluidsynth picks a
  // default driver instead
  explicit FluidSettings( int midi_channels = 0,  int cpu_cores = 0,
                         const char * audio_driver = nullptr);

  NO_MOVE_COPY(FluidSettings)

  ~FluidSettings();
};
