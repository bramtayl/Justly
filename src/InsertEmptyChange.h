#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "justly/global.h"

class ChordsModel;  // lines 12-12

class JUSTLY_EXPORT InsertEmptyChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  int first_child_number;
  int number_of_children;
  int parent_number;

 public:
  explicit InsertEmptyChange(ChordsModel*, int, int, int,
                             QUndoCommand* = nullptr);

  void undo() override;
  void redo() override;
};
