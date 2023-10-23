#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include <gsl/pointers>

#include "justly/utilities/SongIndex.h"  // for SongIndex

class ChordsModel;  // lines 12-12

class CellChange : public QUndoCommand {
 private:
  gsl::not_null<ChordsModel *> chords_model_pointer;
  SongIndex song_index;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit CellChange(gsl::not_null<ChordsModel *>, const SongIndex &, QVariant,
                      QVariant, QUndoCommand * = nullptr);

  void undo() override;
  void redo() override;
};
