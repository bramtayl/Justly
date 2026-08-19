#include "sound/FluidSettings.hpp"

#include <fluidsynth.h>
#include <fluidsynth/misc.h>
#include <fluidsynth/settings.h>

#include <QtCore/QtAssert>

FluidSettings::FluidSettings(const int midi_channels, const int cpu_cores,
                             const char* const audio_driver)
    : internal_pointer(new_fluid_settings()) {
  Q_ASSERT(internal_pointer != nullptr);
  if (midi_channels > 0) {
    auto midi_channels_was_set =
        fluid_settings_setint(internal_pointer, "synth.midi-channels",
                              midi_channels) == FLUID_OK;
    Q_ASSERT(midi_channels_was_set);
  }
  if (cpu_cores > 0) {
    auto cpu_cores_was_set =
        fluid_settings_setint(internal_pointer, "synth.cpu-cores", cpu_cores) ==
        FLUID_OK;
    Q_ASSERT(cpu_cores_was_set);
  }
  if (audio_driver != nullptr) {
    auto audio_driver_was_set =
        fluid_settings_setstr(internal_pointer, "audio.driver", audio_driver) ==
        FLUID_OK;
    Q_ASSERT(audio_driver_was_set);
  }
}

FluidSettings::~FluidSettings() { delete_fluid_settings(internal_pointer); }
