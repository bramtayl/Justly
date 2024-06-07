#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include "justly/macros.h"
#include "justly/SongIndex.h"  // for SongIndex

class ChordsModel;  // lines 12-12

class CellChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  SongIndex song_index;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit CellChange(ChordsModel*, const SongIndex&, QVariant, QVariant,
                      QUndoCommand* = nullptr);

  NO_MOVE_COPY(CellChange);

  void undo() override;
  void redo() override;
};
