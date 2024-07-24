#pragma once

#include "justly/NoteChordField.hpp"
#include <cstddef>

struct CellIndex {
  size_t child_number;
  NoteChordField note_chord_field;
  int parent_number;
  CellIndex(size_t child_number_input, NoteChordField note_chord_field_input,
             int parent_number_input);
};