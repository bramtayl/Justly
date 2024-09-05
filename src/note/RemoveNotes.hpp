#pragma once

#include <QUndoStack>
#include <cstddef>
#include <vector>

#include "note/Note.hpp"

struct NotesModel;

struct RemoveNotes : public QUndoCommand {
  NotesModel *const notes_model_pointer;
  const size_t first_note_number;
  const std::vector<Note> old_notes;

  RemoveNotes(NotesModel *notes_model_pointer_input,
              size_t first_note_number_input,
              const std::vector<Note> &old_notes_input,
              QUndoCommand *parent_pointer_input = nullptr);

  void undo() override;
  void redo() override;
};
