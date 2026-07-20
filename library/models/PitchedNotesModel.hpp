#pragma once

#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"

struct PitchedNotesModel : public UndoRowsModel<PitchedNote> {
  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song)
      : UndoRowsModel<PitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(const int row_number,
                                      const int column_number) const
      -> QVariant override {
    if (column_number == pitched_note_voice_number_column) {
      return song.pitched_voices.at(get_rows().at(row_number).voice_number)
          .name;
    }
    return UndoRowsModel<PitchedNote>::get_display_data(row_number,
                                                        column_number);
  }

  void add_to_status(QTextStream &stream, const int /*row_number*/,
                     const PitchedNote &pitched_note) const override {
    auto play_state = get_play_state_at_chord(song, parent_chord_number);
    add_frequency_to_stream(stream,
                            play_state.current_key *
                                interval_to_double(pitched_note.interval));
    add_timing_to_stream(
        stream, play_state,
        play_state.current_velocity *
            rational_to_double(pitched_note.velocity_ratio) *
            rational_to_double(song.pitched_voices.at(pitched_note.voice_number)
                                   .volume_ratio),
        rational_to_double(pitched_note.beats));
  }
};
