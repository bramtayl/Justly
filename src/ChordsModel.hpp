#pragma once

#include "Song.hpp"
#include "UndoRowsModel.hpp"

struct ChordsModel : public UndoRowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input)
      : UndoRowsModel(undo_stack), song(song_input) {}

  [[nodiscard]] auto get_rows() const -> QList<Chord> & override {
    return song.chords;
  };

  [[nodiscard]] auto
  get_status(const int row_number) const -> QString override {
    return get_status_text(song, row_number);
  }
};
