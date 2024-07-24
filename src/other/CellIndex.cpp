#include "justly/CellIndex.hpp"

CellIndex::CellIndex(size_t child_number_input,
                       NoteChordField note_chord_field_input,
                       int parent_number_input)
    : child_number(child_number_input),
      note_chord_field(note_chord_field_input),
      parent_number(parent_number_input) {}