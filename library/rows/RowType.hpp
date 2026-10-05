#pragma once

#include <cstdint>

enum class RowType : std::uint8_t {
  chord_type,
  pitched_note_type,
  unpitched_note_type,
  pitched_voice_type,
  unpitched_voice_type
};

[[nodiscard]] inline auto is_note_type(const RowType row_type) -> bool {
  return row_type == RowType::pitched_note_type ||
         row_type == RowType::unpitched_note_type;
}

[[nodiscard]] inline auto is_voice_type(const RowType row_type) -> bool {
  return row_type == RowType::pitched_voice_type ||
         row_type == RowType::unpitched_voice_type;
}
