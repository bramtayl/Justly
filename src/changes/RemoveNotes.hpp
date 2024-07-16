#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "justly/ChordsModel.hpp"
#include "justly/Note.hpp"

class RemoveNotes : public QUndoCommand {
  ChordsModel *const chords_model_pointer;
  const size_t chord_number;
  const size_t first_note_number;
  const std::vector<Note> notes;

public:
  RemoveNotes(ChordsModel *chords_model_pointer_input,
              size_t chord_number_input,
              size_t first_note_number_input,
              const std::vector<Note> &notes_input, 
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
