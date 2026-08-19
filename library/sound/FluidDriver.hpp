#pragma once

#include <fluidsynth/types.h>

#include "other/helpers.hpp"

struct FluidDriver {
  fluid_audio_driver_t* internal_pointer;

  explicit FluidDriver(fluid_audio_driver_t* internal_pointer_input)
      : internal_pointer(internal_pointer_input) {}

  NO_COPY(FluidDriver)

  void reset();

  auto operator=(FluidDriver&& other) noexcept -> FluidDriver&;

  FluidDriver(FluidDriver&& other) noexcept;

  ~FluidDriver();
};
