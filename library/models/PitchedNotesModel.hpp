#pragma once

#include "models/NotesModel.hpp"
#include "other/Song.hpp"

struct PitchedNotesModel : public NotesModel<PitchedNote> {
  Song &song;

  explicit PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
      : NotesModel<PitchedNote>(undo_stack), song(song_input) {}

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    const auto &note = get_reference(rows_pointer).at(row_number);
    return get_status_text(song, parent_chord_number,
                           interval_to_double(note.interval),
                           rational_to_double(note.velocity_ratio));
  }
};
