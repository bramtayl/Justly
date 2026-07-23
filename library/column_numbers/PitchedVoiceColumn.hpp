#pragma once

#include <cstdint>

enum class PitchedVoiceColumn : std::uint8_t {
  pitched_voice_instrument_column,
  pitched_voice_velocity_ratio_column,
  pitched_voice_name_column,
  number_of_pitched_voice_columns
};