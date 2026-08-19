#pragma once

#include <QtCore/QtAssert>
#include <fluidsynth.h>
#include <fluidsynth/seq.h>
#include <fluidsynth/seqbind.h>
#include <fluidsynth/types.h>

#include "other/helpers.hpp"
#include "sound/FluidSynth.hpp"

// asserts before registering so a failed new_fluid_sequencer2 (e.g. OOM)
// never gets passed into fluid_sequencer_register_fluidsynth -- a member
// initializer can't be preceded by a constructor-body assert, since member
// initializers all run before the body does
[[nodiscard]] auto
register_fluidsynth_client(fluid_sequencer_t *const sequencer_pointer,
                           FluidSynth &synth) -> fluid_seq_id_t;

struct FluidSequencer {
  fluid_sequencer_t *const internal_pointer;
  const fluid_seq_id_t sequencer_id;

  explicit FluidSequencer(FluidSynth &synth)
      : internal_pointer(new_fluid_sequencer2(0)),
        sequencer_id(register_fluidsynth_client(internal_pointer, synth)) {}

  NO_MOVE_COPY(FluidSequencer)

  ~FluidSequencer();
};
