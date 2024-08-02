#include "other/conversions.hpp"
#include "other/private_constants.hpp"
#include <QtGlobal>
#include <cstddef>

auto to_size_t(int input) -> size_t {
  Q_ASSERT(input >= 0);
  return static_cast<size_t>(input);
}

auto to_note_chord_column(int column) -> NoteChordColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_NOTE_CHORD_COLUMNS);
  return static_cast<NoteChordColumn>(column);
}
