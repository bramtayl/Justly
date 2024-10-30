#pragma once

#include <QString>

#include "chord/Chord.hpp"
#include "rows/RowsModel.hpp"

class QObject;
class QModelIndex;
class QUndoStack;
struct Song;

struct ChordsModel : public RowsModel<Chord> {
  Song &song;

  explicit ChordsModel(QUndoStack& undo_stack,
                       Song &song_input,
                       QObject *parent_pointer = nullptr);

  // override functions
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto
  is_column_editable(int column_number) const -> bool override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};

