#include "song/EditNotes.hpp"

#include <QtGlobal>
#include <QList>
#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"
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
  notes_to_chords(song_editor_pointer);
}

void EditNotes::redo() {
  edit_notes_directly(song_editor_pointer, chord_number);
}
