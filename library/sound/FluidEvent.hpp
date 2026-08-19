#pragma once

#include <fluidsynth/types.h>

#include "other/helpers.hpp"

struct FluidEvent {
  fluid_event_t *const internal_pointer;

  FluidEvent();

  NO_MOVE_COPY(FluidEvent)

  ~FluidEvent();
};
