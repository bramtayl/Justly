#pragma once

#include <cstddef>

#include "justly/NoteChordField.hpp"

struct CellIndex {
  const int parent_number;
  const size_t child_number;
  const NoteChordField note_chord_field;

  CellIndex(int parent_number_input, size_t child_number_input,
            NoteChordField note_chord_field_input = symbol_column);
  [[nodiscard]] auto operator==(const CellIndex &other_index) const -> bool;
};
