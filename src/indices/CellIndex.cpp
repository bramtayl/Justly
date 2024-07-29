#include "indices/CellIndex.hpp"
#include "other/conversions.hpp"

CellIndex::CellIndex(size_t child_number_input,
                     NoteChordColumn note_chord_column_input,
                     int parent_number_input)
    : child_number(child_number_input),
      note_chord_column(note_chord_column_input),
      parent_number(parent_number_input) {}

auto CellIndex::is_chords() const -> bool { return parent_number == -1; }

auto CellIndex::get_parent_chord_number() const -> size_t {
  return to_size_t(parent_number);
}