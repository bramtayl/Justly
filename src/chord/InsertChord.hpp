#pragma once

#include <QUndoStack>
#include <cstddef>

#include "chord/Chord.hpp"

struct ChordsModel;

struct InsertChord : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const Chord new_chord;

  InsertChord(ChordsModel *chords_model_pointer_input,
              size_t chord_number_input, const Chord &new_chord_input,
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
