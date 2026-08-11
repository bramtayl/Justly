#pragma once

#include <QtCore/QtAssert>
#include <fluidsynth.h>
#include <fluidsynth/event.h>
#include <fluidsynth/types.h>

#include "other/helpers.hpp"

struct FluidEvent {
  fluid_event_t *const internal_pointer;

  FluidEvent() : internal_pointer(new_fluid_event()) {
    Q_ASSERT(internal_pointer != nullptr);
  }

  NO_MOVE_COPY(FluidEvent)

  ~FluidEvent() { delete_fluid_event(internal_pointer); }
};
