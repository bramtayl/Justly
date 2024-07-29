#pragma once

#include "justly/NoteChordColumn.hpp"
#include <cstddef>

#include "justly/JUSTLY_EXPORT.hpp"

struct JUSTLY_EXPORT CellIndex {
  const size_t child_number;
  const NoteChordColumn note_chord_column;
  const int parent_number;

  CellIndex(size_t child_number_input, NoteChordColumn note_chord_column_input,
            int parent_number_input);
  [[nodiscard]] auto is_chords() const -> bool;
  [[nodiscard]] auto get_parent_chord_number() const -> size_t;
};