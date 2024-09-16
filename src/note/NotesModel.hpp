#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <QList> // IWYU pragma: keep

#include "chord/ChordsModel.hpp"
#include "other/ItemModel.hpp"
#include "justly/NoteColumn.hpp"
#include "note/Note.hpp" // IWYU pragma: keep

class QUndoStack;
class QWidget;

[[nodiscard]] auto to_note_column(int column) -> NoteColumn;

struct NotesModel : public ItemModel {
  QWidget *const parent_pointer;
  ChordsModel* const parent_chords_model_pointer;
  int parent_chord_number = -1;
  QList<Note>* notes_pointer = nullptr;

  explicit NotesModel(ChordsModel* parent_chords_model_pointer_input,
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
};

void insert_notes(NotesModel& notes_model, qsizetype first_note_number,
                  const QList<Note> &new_notes);
void remove_notes(NotesModel& notes_model, qsizetype first_note_number,
                  qsizetype number_of_notes);