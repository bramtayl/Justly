#pragma once

#include <QString>

#include "row/pitched_note/PitchedNote.hpp"
#include "row/RowsModel.hpp"

class QObject;
class QModelIndex;
class QUndoStack;
struct Song;

struct PitchedNotesModel : public RowsModel<PitchedNote> {
  Song& song;
  int parent_chord_number = -1;

  explicit PitchedNotesModel(QUndoStack& undo_stack, Song& song_input);
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};