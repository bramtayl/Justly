#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include "justly/global.h"
#include "src/SongIndex.h"  // for SongIndex

class ChordsModel;  // lines 12-12

class JUSTLY_EXPORT CellChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  SongIndex song_index;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit CellChange(ChordsModel*, const SongIndex&, QVariant, QVariant,
                      QUndoCommand* = nullptr);

  void undo() override;
  void redo() override;
};
