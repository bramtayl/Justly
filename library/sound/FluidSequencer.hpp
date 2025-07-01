#pragma once

#include <fluidsynth.h>
#include <fluidsynth/types.h>

#include "other/helpers.hpp"
#include "sound/FluidSynth.hpp"

struct FluidSequencer {
  fluid_sequencer_t *const internal_pointer;
  const fluid_seq_id_t sequencer_id;

  explicit FluidSequencer(FluidSynth &synth)
      : internal_pointer(new_fluid_sequencer2(0)),
        sequencer_id(fluid_sequencer_register_fluidsynth(
            internal_pointer, synth.internal_pointer)) {}

  NO_MOVE_COPY(FluidSequencer)

  ~FluidSequencer() {
    fluid_sequencer_unregister_client(internal_pointer, sequencer_id);
    delete_fluid_sequencer(internal_pointer);
  }
};