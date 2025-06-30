#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidEvent {
  fluid_event_t *const internal_pointer;

  FluidEvent() : internal_pointer(new_fluid_event()) {}

  NO_MOVE_COPY(FluidEvent)

  ~FluidEvent() { delete_fluid_event(internal_pointer); }
};
