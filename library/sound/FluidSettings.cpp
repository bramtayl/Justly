#include "sound/FluidSettings.hpp"

FluidSettings::FluidSettings(const int midi_channels, const int cpu_cores,
                             const char* const audio_driver)
    : CHandle(new_fluid_settings()) {
  Q_ASSERT(internal_pointer != nullptr);
  if (midi_channels > 0) {
    set_fluid_int(*this, "synth.midi-channels", midi_channels);
  }
  if (cpu_cores > 0) {
    set_fluid_int(*this, "synth.cpu-cores", cpu_cores);
  }
  if (audio_driver != nullptr) {
    set_fluid_string(*this, "audio.driver", audio_driver);
  }
}

void check_fluid_ok(const int fluid_result) {
  Q_ASSERT(fluid_result == FLUID_OK);
}

void set_fluid_int(FluidSettings& settings, const char* const field,
                   const int value) {
  Q_ASSERT(field != nullptr);
  check_fluid_ok(
      fluid_settings_setint(settings.internal_pointer, field, value));
}

void set_fluid_string(FluidSettings& settings, const char* const field,
                      const char* const value) {
  Q_ASSERT(field != nullptr);
  Q_ASSERT(value != nullptr);
  check_fluid_ok(
      fluid_settings_setstr(settings.internal_pointer, field, value));
}
