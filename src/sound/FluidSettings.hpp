#pragma once

#include <fluidsynth.h>

struct FluidSettings {
  fluid_settings_t *internal_pointer;

  FluidSettings() : internal_pointer(new_fluid_settings()) {}

  FluidSettings(const FluidSettings &) = delete;
  auto operator=(const FluidSettings &) -> FluidSettings & = delete;
  auto operator=(FluidSettings &&other) noexcept -> FluidSettings & = delete;

  FluidSettings(FluidSettings &&other) noexcept
      : internal_pointer(other.internal_pointer) {
    other.internal_pointer = nullptr;
  }

  ~FluidSettings() {
    if (internal_pointer != nullptr) {
      delete_fluid_settings(internal_pointer);
    }
  }
};
