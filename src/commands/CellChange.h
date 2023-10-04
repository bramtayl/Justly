#pragma once

#include <qtmetamacros.h>  // for signals
#include <qundostack.h>    // for QUndoCommand
#include <qvariant.h>      // for QVariant

#include <gsl/pointers>

#include "utilities/StableIndex.h"  // for StableIndex

class Editor;  // lines 12-12
class QModelIndex;

class CellChange : public QUndoCommand {
 private:
  gsl::not_null<Editor *> editor_pointer;
  StableIndex stable_index;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit CellChange(gsl::not_null<Editor *> editor_pointer_input,
                      const QModelIndex &index_input, QVariant old_value_input,
                      QVariant new_value_input,
                      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
 signals:
  void should_change_cell(const QModelIndex &index, QVariant old_value,
                         QVariant new_value);
};
