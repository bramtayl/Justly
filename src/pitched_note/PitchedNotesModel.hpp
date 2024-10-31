#pragma once

#include <QString>

#include "pitched_note/PitchedNote.hpp"
#include "rows/RowsModel.hpp"

class QObject;
class QModelIndex;
class QUndoStack;
struct Song;

struct PitchedNotesModel : public RowsModel<PitchedNote> {
  Song& song;
  int parent_chord_number = -1;

  explicit PitchedNotesModel(QUndoStack& undo_stack, Song& song_input,
                             QObject *parent_pointer = nullptr);
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
