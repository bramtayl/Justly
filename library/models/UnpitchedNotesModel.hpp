#pragma once

#include "models/UndoRowsModel.hpp"
#include "other/Song.hpp"
#include "rows/UnpitchedNote.hpp"

struct UnpitchedNotesModel : public UndoRowsModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack, Song &song)
      : UndoRowsModel<UnpitchedNote>(undo_stack, song) {}

  [[nodiscard]] auto get_display_data(const int row_number,
                                      const int column_number) const
      -> QVariant override {
    if (column_number == unpitched_note_voice_number_column) {
      return song.unpitched_voices.at(get_rows().at(row_number).voice_number)
          .name;
    }
    return UndoRowsModel<UnpitchedNote>::get_display_data(row_number,
                                                          column_number);
  }

  void add_to_status(QTextStream &stream, const int /*row_number*/,
                     const UnpitchedNote &unpitched_note) const override {
    auto play_state = get_play_state_at_chord(song, parent_chord_number);
    add_timing_to_stream(
        stream, play_state,
        play_state.current_velocity *
            rational_to_double(unpitched_note.velocity_ratio) *
            rational_to_double(
                song.unpitched_voices.at(unpitched_note.voice_number)
                    .velocity_ratio),
        rational_to_double(unpitched_note.beats));
  }
};