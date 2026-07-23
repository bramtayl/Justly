#pragma once

#include <cstdint>

enum class UnpitchedVoiceColumn : std::uint8_t {
  unpitched_voice_percussion_set_column,
  unpitched_voice_midi_number_column,
  unpitched_voice_velocity_ratio_column,
  unpitched_voice_name_column,
  number_of_unpitched_voice_columns
};