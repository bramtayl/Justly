#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <gsl/pointers>

#include "utilities/SongIndex.h"  // for SongIndex

class ChordsModel;  // lines 12-12

class InsertEmptyChange : public QUndoCommand {
 private:
  gsl::not_null<ChordsModel *> chords_model_pointer;
  int first_child_number;
  int number_of_children;
  SongIndex parent_song_index;

 public:
  explicit InsertEmptyChange(
      gsl::not_null<ChordsModel *>,
      int, int,
      const SongIndex &,
      QUndoCommand * = nullptr);

  void undo() override;
  void redo() override;
};
