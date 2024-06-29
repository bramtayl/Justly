#pragma once

#include "justly/public_constants.hpp"

enum JUSTLY_EXPORT NoteChordField {
  symbol_column,
  instrument_column,
  interval_column,
  beats_column,
  volume_ratio_column,
  tempo_ratio_column,
  words_column
};

auto to_note_chord_field(int column) -> NoteChordField;