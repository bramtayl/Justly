#include "justly/NoteChordColumn.hpp"

#include <QtGlobal>

auto to_note_chord_column(int column) -> NoteChordColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_CHORD_COLUMNS);
  return static_cast<NoteChordColumn>(column);
}