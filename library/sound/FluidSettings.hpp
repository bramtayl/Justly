#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidSettings : CHandle<fluid_settings_t, delete_fluid_settings> {
  // midi_channels/cpu_cores/audio_driver must be set (when wanted) before
  // the settings are handed to new_fluid_synth() -- left at their defaults,
  // no playback config is applied, matching plain fluidsynth defaults;
  // audio_driver is null on non-Linux platforms, where fluidsynth picks a
  // default driver instead
  explicit FluidSettings(int midi_channels = 0, int cpu_cores = 0,
                         const char* audio_driver = nullptr);
};

void check_fluid_ok(int fluid_result);

void set_fluid_int(FluidSettings& settings, const char* field, int value);

void set_fluid_string(FluidSettings& settings, const char* field,
                      const char* value);
