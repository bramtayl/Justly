#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QSize>
#include <QTreeView>
#include <QVariant>
#include <cstddef>

#include "justly/NoteChordColumn.hpp"

class ChordsModel;
class QUndoStack;
class QWidget;

struct ChordsView : public QTreeView {
  Q_OBJECT
public:
  ChordsModel *const chords_model_pointer;

  explicit ChordsView(QUndoStack *undo_stack_pointer_input,
                      QWidget *parent = nullptr);
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;

  [[nodiscard]] auto get_chord_index(
      size_t chord_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;
  [[nodiscard]] auto get_note_index(
      size_t chord_number, size_t note_number,
      NoteChordColumn note_chord_column = type_column) const -> QModelIndex;

  [[nodiscard]] auto create_editor(QModelIndex index) -> QWidget *;
  void set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                  const QVariant &new_value);

  void copy_selected();
  void paste_cells_or_after();
  void paste_into();

  void insert_after();
  void insert_into();
  void delete_selected();

  void expand_selected();
  void collapse_selected();
};
