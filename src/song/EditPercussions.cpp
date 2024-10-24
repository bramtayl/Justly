#include "song/EditPercussions.hpp"

#include "song/SongEditor.hpp"
#include <QtGlobal>

EditPercussions::EditPercussions(SongEditor *song_editor_pointer_input,
                                 int chord_number_input,
                                 QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      song_editor_pointer(song_editor_pointer_input),
      chord_number(chord_number_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void EditPercussions::undo() { song_editor_pointer->percussions_to_chords(); }

void EditPercussions::redo() {
  song_editor_pointer->edit_percussions(chord_number);
}
