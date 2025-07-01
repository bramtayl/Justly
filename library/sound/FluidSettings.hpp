#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidSettings {
  fluid_settings_t *internal_pointer;

  FluidSettings() : internal_pointer(new_fluid_settings()) {}

  NO_COPY(FluidSettings)
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
