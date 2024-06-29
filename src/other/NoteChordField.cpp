#include "justly/NoteChordField.hpp"

#include <qassert.h>  // for Q_ASSERT

#include "justly/public_constants.hpp"  // for NOTE_CHORD_COLUMNS

auto to_note_chord_field(int column) -> NoteChordField {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NOTE_CHORD_COLUMNS);
  return static_cast<NoteChordField>(column);
}