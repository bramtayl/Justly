#include "song/NotesToChords.hpp"

#include <QtGlobal>
#include "song/SongEditor.hpp"


NotesToChords::NotesToChords(
    SongEditor *song_editor_pointer_input,
    qsizetype chord_number_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      song_editor_pointer(song_editor_pointer_input),
      chord_number(chord_number_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void NotesToChords::undo() {
  song_editor_pointer->edit_notes_directly(chord_number);
}

void NotesToChords::redo() {
  song_editor_pointer->notes_to_chords();
}
