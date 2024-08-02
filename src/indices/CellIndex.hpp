#pragma once

#include "justly/NoteChordColumn.hpp"

#include <cstddef>

class CellIndex {
private:
  const int parent_number;

public:
  const size_t child_number;
  const NoteChordColumn note_chord_column;

  CellIndex(size_t child_number_input, NoteChordColumn note_chord_column_input,
            int parent_number_input);
  [[nodiscard]] auto is_chords() const -> bool;
  [[nodiscard]] auto get_parent_chord_number() const -> size_t;
};