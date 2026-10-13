#pragma once

#include <cstdint>

enum class PitchedNoteColumn : std::uint8_t {
  voice_name_column,
  interval_column,
  beats_column,
  velocity_ratio_column,
  words_column,
  number_of_columns
};