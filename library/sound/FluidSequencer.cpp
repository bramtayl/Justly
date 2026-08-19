#include "sound/FluidSequencer.hpp"

#include <fluidsynth/seq.h>
#include <fluidsynth/seqbind.h>
#include <fluidsynth/types.h>

#include <QtCore/QtAssert>

#include "sound/FluidSynth.hpp"

auto register_fluidsynth_client(fluid_sequencer_t* const sequencer_pointer,
                                FluidSynth& synth) -> fluid_seq_id_t {
  Q_ASSERT(sequencer_pointer != nullptr);
  return fluid_sequencer_register_fluidsynth(sequencer_pointer,
                                             synth.internal_pointer);
}

FluidSequencer::~FluidSequencer() {
  if (internal_pointer != nullptr) {
    fluid_sequencer_unregister_client(internal_pointer, sequencer_id);
    delete_fluid_sequencer(internal_pointer);
  }
}
