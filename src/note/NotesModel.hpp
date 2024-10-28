#pragma once

#include <QString>

#include "note/Note.hpp"
#include "rows/RowsModel.hpp"

class QObject;
struct ChordsModel;
class QModelIndex;

struct NotesModel : public RowsModel<Note> {
  ChordsModel *const parent_chords_model_pointer;
  int parent_chord_number = -1;

  explicit NotesModel(ChordsModel *parent_chords_model_pointer_input,
                      QObject *parent_pointer = nullptr);

  // override functions
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto
  get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto get_status(int row_number) const -> QString override;
};
