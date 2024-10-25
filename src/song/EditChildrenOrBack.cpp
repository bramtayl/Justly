#include "song/EditChildrenOrBack.hpp"

#include "song/SongEditor.hpp"
#include <QtGlobal>

EditChildrenOrBack::EditChildrenOrBack(SongEditor *song_editor_pointer_input,
                     int chord_number_input, bool is_notes_input,
                     bool backwards_input, QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      song_editor_pointer(song_editor_pointer_input),
      chord_number(chord_number_input),
      is_notes(is_notes_input),
      backwards(backwards_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void EditChildrenOrBack::edit_children(bool should_edit_children) const {
  if (should_edit_children) {
      if (is_notes) {
        song_editor_pointer->edit_notes(chord_number);
      } else {
        song_editor_pointer->edit_percussions(chord_number);
      }
  } else {
      if (is_notes) {
        song_editor_pointer->notes_to_chords();
      } else {
        song_editor_pointer->percussions_to_chords();
      }
  }
};

void EditChildrenOrBack::undo() { edit_children(backwards); }

void EditChildrenOrBack::redo() { edit_children(!backwards); }
