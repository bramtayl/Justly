#pragma once

#include <QString>

#include "chord/Chord.hpp"
#include "rows/RowsModel.hpp"

class QUndoStack;
struct Song;

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack &undo_stack, Song &song_input);
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
