#include "song/PercussionsToChords.hpp"

#include <QtGlobal>
#include <QList>
#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion/PercussionsModel.hpp"
#include "song/SongEditor.hpp"


PercussionsToChords::PercussionsToChords(
    SongEditor *song_editor_pointer_input,
    qsizetype chord_number_input,
    QUndoCommand *parent_pointer_input)
    : QUndoCommand(parent_pointer_input),
      song_editor_pointer(song_editor_pointer_input),
      chord_number(chord_number_input) {
  Q_ASSERT(song_editor_pointer != nullptr);
}

void PercussionsToChords::undo() {
  edit_percussions_directly(song_editor_pointer, chord_number);
}

void PercussionsToChords::redo() {
  percussions_to_chords(song_editor_pointer);
}
