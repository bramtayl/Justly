#pragma once

#include <QUndoStack>
#include <cstddef>

#include "note_chord/Note.hpp"

struct ChordsModel;

struct InsertNote : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t note_number;
  const Note new_note;

  InsertNote(ChordsModel *chords_model_pointer_input, size_t chord_number_input,
             size_t note_number_input, const Note &new_note_input,
             QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
