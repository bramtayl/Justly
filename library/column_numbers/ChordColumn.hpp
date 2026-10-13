#pragma once

#include <cstdint>

enum class ChordColumn : std::uint8_t {
  pitched_notes_column,
  unpitched_notes_column,
  interval_column,
  beats_column,
  velocity_ratio_column,
  tempo_ratio_column,
  words_column,
  number_of_columns
};