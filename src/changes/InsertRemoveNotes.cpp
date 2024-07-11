#include "changes/InsertRemoveNotes.hpp"

#include <QtGlobal>

#include "justly/ChordsModel.hpp"

InsertRemoveNotes::InsertRemoveNotes(ChordsModel* chords_model_pointer_input,
                                     size_t first_child_number_input,
                                     const std::vector<Note>& notes_input,
                                     int parent_number_input,
                                     bool is_insert_input,
                                     QUndoCommand* parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      chords_model_pointer(chords_model_pointer_input),
      first_child_number(first_child_number_input),
      notes(notes_input),
      parent_number(parent_number_input),
      is_insert(is_insert_input) {}

auto InsertRemoveNotes::undo() -> void { insert_or_remove(!is_insert); }

auto InsertRemoveNotes::redo() -> void { insert_or_remove(is_insert); }

void InsertRemoveNotes::insert_or_remove(bool should_insert) {
  Q_ASSERT(chords_model_pointer != nullptr);
  chords_model_pointer->insert_remove_notes(first_child_number, notes,
                                            parent_number, should_insert);
}
