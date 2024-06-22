#pragma once

#include <qundostack.h>  // for QUndoCommand
#include <qvariant.h>    // for QVariant

#include "justly/SongIndex.hpp"  // for SongIndex

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

  [[nodiscard]] auto id() const -> int override;
  auto mergeWith(const QUndoCommand* next_command_pointer) -> bool override;
  
  void undo() override;
  void redo() override;
};