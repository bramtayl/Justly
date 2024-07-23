#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QSize>
#include <QTreeView>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>

#include "justly/ChordsModel.hpp"
#include "justly/JUSTLY_EXPORT.hpp"

struct JUSTLY_EXPORT ChordsView : public QTreeView {
  Q_OBJECT
public:
  ChordsModel *const chords_model_pointer;
  QUndoStack *const undo_stack_pointer;

  explicit ChordsView(QUndoStack *undo_stack_pointer_input,
                      QWidget *parent = nullptr);
  [[nodiscard]] auto viewportSizeHint() const -> QSize override;

  [[nodiscard]] auto create_editor(QModelIndex index) -> QWidget *;
  void set_editor(QWidget *cell_editor_pointer, QModelIndex index,
                  const QVariant &new_value);

  void copy_selected();
  void paste_cells_or_after();
  void paste_into();

  void insert_after();
  void insert_into();
  void delete_selected();
};
