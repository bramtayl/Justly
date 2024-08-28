#pragma once

#include <QUndoStack>
#include <cstddef>

#include "note_chord/Chord.hpp"

struct ChordsModel;

class InsertChord : public QUndoCommand {
private:
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const Chord new_chord;

public:
  InsertChord(ChordsModel *chords_model_pointer_input,
              size_t chord_number_input, const Chord &new_chord_input,
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
