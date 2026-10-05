#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidEvent : CHandle<fluid_event_t, delete_fluid_event> {
  FluidEvent();
};
