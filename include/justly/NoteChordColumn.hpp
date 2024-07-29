#pragma once

#include "justly/JUSTLY_EXPORT.hpp"

enum JUSTLY_EXPORT NoteChordColumn {
  type_column,
  instrument_column,
  interval_column,
  beats_column,
  volume_ratio_column,
  tempo_ratio_column,
  words_column
};

const auto NUMBER_OF_NOTE_CHORD_COLUMNS = 7;

[[nodiscard]] auto to_note_chord_column(int column) -> NoteChordColumn;