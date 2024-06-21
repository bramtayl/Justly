#pragma once

#include "justly/global.hpp"
#include "justly/NoteChordField.hpp"

struct JUSTLY_EXPORT SongIndex {
  int chord_number;
  int note_number;
  int note_chord_field;

  [[nodiscard]] auto operator==(const SongIndex &other_index) const -> bool;
};
