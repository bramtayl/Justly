#pragma once

#include <fluidsynth.h>

#include "other/helpers.hpp"

struct FluidEvent {
  fluid_event_t * const internal_pointer;

  FluidEvent() : internal_pointer(new_fluid_event()) {}

  NO_MOVE_COPY(FluidEvent)

  ~FluidEvent() {
    delete_fluid_event(internal_pointer);
  }
};

static inline void set_destination(FluidEvent &event,
                                   const fluid_seq_id_t sequencer_id) {
  fluid_event_set_dest(event.internal_pointer, sequencer_id);
}