#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <cstddef>
#include <vector>

#include "justly/NoteColumn.hpp"

struct Note;
class QUndoStack;
class QWidget;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> size_t;

[[nodiscard]] auto to_note_column(int column) -> NoteColumn;

struct NotesModel : public QAbstractTableModel {
  QWidget *const parent_pointer;
  std::vector<Note> &notes;
  QUndoStack *const undo_stack_pointer;

  explicit NotesModel(std::vector<Note> &notes_input,
                      QUndoStack *undo_stack_pointer_input,
                      QWidget *parent_pointer_input = nullptr);

  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent_index*/) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex & /*parent_index*/) const -> int override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;

  // internal functions
  void edited_notes_cells(size_t first_note_number, size_t number_of_notes,
                          NoteColumn left_column, NoteColumn right_column);
  void begin_insert_rows(size_t first_note_number, size_t number_of_notes);
  void end_insert_rows();

  void begin_remove_rows(size_t first_note_number, size_t number_of_notes);
  void end_remove_rows();
};

void insert_notes(NotesModel *notes_model_pointer, size_t first_note_number,
                  const std::vector<Note> &new_notes);
void remove_notes(NotesModel *notes_model_pointer, size_t first_note_number,
                  size_t number_of_notes);