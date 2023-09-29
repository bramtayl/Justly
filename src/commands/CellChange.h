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
  bool first_time = true;

 public:
  explicit CellChange(gsl::not_null<Editor *> editor_pointer_input,
                      const QModelIndex &index_input, QVariant old_value_input,
                      QVariant new_value_input,
                      QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
 signals:
  void about_to_set_data(const QModelIndex &index, QVariant old_value,
                         QVariant new_value);
};
