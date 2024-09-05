#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "chord/Chord.hpp"
#include "justly/ChordColumn.hpp"

struct ChordsModel;

struct SetChordsCells : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t first_chord_number;
  const ChordColumn left_column;
  const ChordColumn right_column;
  const std::vector<Chord> old_chords;
  const std::vector<Chord> new_chords;
  explicit SetChordsCells(ChordsModel *chords_model_pointer_input,
                          size_t first_chord_number_input,
                          ChordColumn left_column_input,
                          ChordColumn right_column_input,
                          const std::vector<Chord> &old_chords_input,
                          const std::vector<Chord> &new_chords_input,
                          QUndoCommand *parent_pointer_input = nullptr);
  void undo() override;
  void redo() override;
};
