#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "note_chord/Chord.hpp"

class ChordsModel;

class InsertChords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t first_chord_number;
  const std::vector<Chord> new_chords;

public:
  InsertChords(ChordsModel *chords_model_pointer_input,
               size_t first_chord_number_input,
               const std::vector<Chord> &new_chords_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
