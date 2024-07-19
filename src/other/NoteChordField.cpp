#include "justly/NoteChordField.hpp"

#include <QtGlobal>

#include "other/private.hpp"

auto to_note_chord_field(int column) -> NoteChordField {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NOTE_CHORD_COLUMNS);
  return static_cast<NoteChordField>(column);
}