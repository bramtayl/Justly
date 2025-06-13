#pragma once

#include <fluidsynth.h>

struct FluidDriver {
  fluid_audio_driver_t *internal_pointer;

  explicit FluidDriver(fluid_audio_driver_t * internal_pointer_input) : internal_pointer(internal_pointer_input) {}

  FluidDriver(const FluidDriver &) = delete;
  auto operator=(const FluidDriver &) -> FluidDriver & = delete;

  auto operator=(FluidDriver &&other) noexcept -> FluidDriver & {
    internal_pointer = other.internal_pointer;
    other.internal_pointer = nullptr;
    return *this;
  }

  FluidDriver(FluidDriver &&other) noexcept
      : internal_pointer(other.internal_pointer) {
    other.internal_pointer = nullptr;
  }

  ~FluidDriver() {
    if (internal_pointer != nullptr) {
        delete_fluid_audio_driver(internal_pointer);
    }
  }
};
