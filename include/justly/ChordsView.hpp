#pragma once

#include <qsize.h>         // for QSize
#include <qtmetamacros.h>  // for Q_OBJECT
#include <qtreeview.h>     // for QTreeView
#include <qvariant.h>      // for QVariant

class ChordsModel;
class QModelIndex;
class QUndoStack;
class QWidget;

struct ChordsView : public QTreeView {
  Q_OBJECT
 public:
  ChordsModel* const chords_model_pointer;
  QUndoStack* const undo_stack_pointer;

  explicit ChordsView(QUndoStack* undo_stack_pointer_input,
                      QWidget* parent = nullptr);
  [[nodiscard]] auto sizeHintForColumn(int column) const -> int override;
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;

  [[nodiscard]] auto create_editor(QModelIndex index) -> QWidget*;
  void set_editor(QWidget* cell_editor_pointer, QModelIndex index,
                  const QVariant& new_value);

  void copy_selected();

  void paste_cell();
  void paste_before();
  void paste_after();
  void paste_into();

  void insert_before();
  void insert_after();
  void insert_into();
  void remove_selected();
};
