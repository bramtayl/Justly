#pragma once

#include "models/UndoRowsModel.hpp"
#include "rows/Note.hpp"

template <NoteInterface SubNote>
struct NotesModel : public UndoRowsModel<SubNote> {
  QList<SubNote> *rows_pointer = nullptr;
  int parent_chord_number = -1;

  explicit NotesModel(QUndoStack &undo_stack)
      : UndoRowsModel<SubNote>(undo_stack) {}

  [[nodiscard]] auto is_valid() const -> bool override {
    return rows_pointer != nullptr;
  };

  [[nodiscard]] auto get_rows() const -> QList<SubNote> & override {
    return get_reference(rows_pointer);
  };

  void set_rows_pointer(QList<SubNote> *const new_rows_pointer = nullptr,
                        const int new_parent_chord_number = -1) {
    NotesModel::beginResetModel();
    rows_pointer = new_rows_pointer;
    parent_chord_number = new_parent_chord_number;
    NotesModel::endResetModel();
  }
};