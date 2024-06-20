#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include "song/SongIndex.hpp"  // for SongIndex

class ChordsModel;  // lines 12-12

class CellChange : public QUndoCommand {
  ChordsModel* chords_model_pointer;
  SongIndex song_index;
  QVariant old_value;
  QVariant new_value;

 public:
  explicit CellChange(ChordsModel* chords_model_pointer_input,
                      const SongIndex& song_index_input,
                      QVariant old_value_input, QVariant new_value_input,
                      QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
