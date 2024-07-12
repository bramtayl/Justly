#pragma once

#include <cstddef>

#include "justly/NoteChordField.hpp"

struct CellIndex {
  const size_t child_number;
  const int parent_number;
  const NoteChordField note_chord_field;

  CellIndex(size_t child_number_input, int parent_number_input,
            NoteChordField note_chord_field_input = symbol_column);
  [[nodiscard]] auto operator==(const CellIndex &other_index) const -> bool;
};
