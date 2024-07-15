#include "changes/RemoveNotes.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"

RemoveNotes::RemoveNotes(ChordsModel* chords_model_pointer_input,
                                     size_t first_child_number_input,
                                     const std::vector<Note>& notes_input,
                                     int parent_number_input,
                                     QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      notes(notes_input),
      parent_number(parent_number_input) {}

auto RemoveNotes::undo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_notes_directly(first_child_number, notes,
                                            parent_number);
}

auto RemoveNotes::redo() -> void {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->remove_notes_directly(first_child_number, notes.size(),
                                            parent_number);
}

