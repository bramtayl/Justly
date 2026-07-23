#pragma once

#include <cstdint>

enum class RowType : std::uint8_t {
  chord_type,
  pitched_note_type,
  unpitched_note_type,
  pitched_voice_type,
  unpitched_voice_type
};
