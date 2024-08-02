#pragma once

#include "justly/NoteChordColumn.hpp"
#include <cstddef>

[[nodiscard]] auto to_size_t(int input) -> size_t;

[[nodiscard]] auto to_note_chord_column(int column) -> NoteChordColumn;
