#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "note_chord/Chord.hpp"

struct ChordsModel;

class RemoveChords : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t first_chord_number;
  const std::vector<Chord> old_chords;

public:
  RemoveChords(ChordsModel *chords_model_pointer_input,
               size_t first_chord_number_input,
               const std::vector<Chord> &old_chords_input,
               QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
