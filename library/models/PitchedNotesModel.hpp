#pragma once

#include "models/NotesModel.hpp"
#include "other/Song.hpp"

struct PitchedNotesModel : public NotesModel<PitchedNote> {
  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song)
      : NotesModel<PitchedNote>(undo_stack, song) {}

  void add_to_status(QTextStream &stream, const int /*row_number*/,
                     const PitchedNote &pitched_note) const override {
    auto play_state = get_play_state_at_chord(song, parent_chord_number);
    add_frequency_to_stream(stream,
                            play_state.current_key *
                                interval_to_double(pitched_note.interval));
    add_timing_to_stream(stream, play_state,
                         play_state.current_velocity *
                             rational_to_double(pitched_note.velocity_ratio),
                         rational_to_double(pitched_note.beats));
  }
};
