#pragma once

#include <qundostack.h>  // for QUndoCommand

#include "justly/macros.h"

class ChordsModel;  // lines 12-12

class InsertEmptyChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  int first_child_number;
  int number_of_children;
  int chord_number;

 public:
  explicit InsertEmptyChange(ChordsModel*, int, int, int,
                             QUndoCommand* = nullptr);
  NO_MOVE_COPY(InsertEmptyChange);

  void undo() override;
  void redo() override;
};
