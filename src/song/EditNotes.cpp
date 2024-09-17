#include "song/EditNotes.hpp"

#include <QtGlobal>
#include "song/SongEditor.hpp"

EditNotes::EditNotes(
    SongEditor *song_editor_pointer_input,
    qsizetype chord_number_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      song_editor_pointer(song_editor_pointer_input),
      chord_number(chord_number_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void EditNotes::undo() {
  song_editor_pointer->notes_to_chords();
}

void EditNotes::redo() {
  song_editor_pointer->edit_notes(chord_number);
}
