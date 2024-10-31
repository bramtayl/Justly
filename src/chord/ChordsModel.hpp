#pragma once

#include <QString>

#include "chord/Chord.hpp"
#include "rows/RowsModel.hpp"

class QObject;
class QUndoStack;
struct Song;

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack& undo_stack,
                       Song &song_input,
                       QObject *parent_pointer = nullptr);

  // override functions
  [[nodiscard]] auto
  is_column_editable(int column_number) const -> bool override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};

