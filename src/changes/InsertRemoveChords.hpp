#pragma once

#include <qundostack.h>  // for QUndoCommand

#include <cstddef>  // for size_t
#include <vector>   // for vector

#include "justly/Chord.hpp"        // for Chord
#include "justly/ChordsModel.hpp"  // for ChordsModel

class InsertRemoveChords : public QUndoCommand {
  ChordsModel* const chords_model_pointer;
  const size_t first_child_number;
  const std::vector<Chord> chords;
  const bool is_insert;

 public:
  InsertRemoveChords(ChordsModel* chords_model_pointer_input,
                     size_t first_child_number_input,
                     const std::vector<Chord>& chords_input,
                     bool is_insert_input,
                     QUndoCommand* parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;

  void insert_or_remove(bool should_insert);
};
