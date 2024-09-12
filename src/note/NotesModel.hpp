#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <QList> // IWYU pragma: keep

#include "justly/NoteColumn.hpp"
#include "note/Note.hpp" // IWYU pragma: keep

class QUndoStack;
class QWidget;

[[nodiscard]] auto get_child_number(const QModelIndex &index) -> qsizetype;

[[nodiscard]] auto to_note_column(int column) -> NoteColumn;

struct NotesModel : public QAbstractTableModel {
  QWidget *const parent_pointer;
  QList<Note>* notes_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit NotesModel(QUndoStack *undo_stack_pointer_input,
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
  void edited_notes_cells(qsizetype first_note_number, qsizetype number_of_notes,
                          NoteColumn left_column, NoteColumn right_column);
  void begin_insert_rows(qsizetype first_note_number, qsizetype number_of_notes);
  void end_insert_rows();

  void begin_remove_rows(qsizetype first_note_number, qsizetype number_of_notes);
  void end_remove_rows();

  void begin_reset_model();
  void end_reset_model();
};

void insert_notes(NotesModel *notes_model_pointer, qsizetype first_note_number,
                  const QList<Note> &new_notes);
void remove_notes(NotesModel *notes_model_pointer, qsizetype first_note_number,
                  qsizetype number_of_notes);