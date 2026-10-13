#pragma once

#include <cstdint>

enum class UnpitchedVoiceColumn : std::uint8_t {
  percussion_set_column,
  midi_number_column,
  velocity_ratio_column,
  name_column,
  number_of_columns
};