#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/Chord.hpp"

class ChordsModel;

class RemoveChords : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t first_chord_number;
  const std::vector<Chord> chords;

public:
  RemoveChords(ChordsModel *chords_model_pointer_input,
               size_t first_chord_number_input,
               const std::vector<Chord> &chords_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
