#include "justly/CellIndex.hpp"

CellIndex::CellIndex(size_t child_number_input, int parent_number_input,
                     NoteChordField note_chord_field_input)
    : child_number(child_number_input),
      parent_number(parent_number_input),
      note_chord_field(note_chord_field_input) {}

auto CellIndex::operator==(const CellIndex& other_index) const -> bool {
  return parent_number == other_index.parent_number &&
         child_number == other_index.child_number &&
         note_chord_field == other_index.note_chord_field;
}