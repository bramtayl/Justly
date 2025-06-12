#pragma once

#include "models/NotesModel.hpp"
#include "rows/UnpitchedNote.hpp"

struct UnpitchedNotesModel : public NotesModel<UnpitchedNote> {
  explicit UnpitchedNotesModel(QUndoStack &undo_stack)
      : NotesModel<UnpitchedNote>(undo_stack) {}
};