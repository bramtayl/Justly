#pragma once

#include "NotesModel.hpp"
#include "UnpitchedNote.hpp"

struct UnpitchedNotesModel : public NotesModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack)
      : NotesModel<UnpitchedNote>(undo_stack) {}
};