#pragma once

#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"

struct ChordsModel : public UndoRowsModel<Chord> {
  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel(undo_stack, song_input) {}

  [[nodiscard]] auto get_rows() const -> QList<Chord> & override {
    return song.chords;
  };

  void add_to_status(QTextStream &stream, const int row_number,
                     const Chord &chord) const override {
    auto play_state = get_play_state_at_chord(song, row_number);
    add_frequency_to_stream(stream, play_state.current_key);
    add_timing_to_stream(stream, play_state, play_state.current_velocity,
                         rational_to_double(chord.beats));
  }
};
