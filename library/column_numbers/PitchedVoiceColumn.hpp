#pragma once

#include <cstdint>

enum class PitchedVoiceColumn : std::uint8_t {
  instrument_column,
  velocity_ratio_column,
  name_column,
  number_of_columns
};