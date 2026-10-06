#pragma once

#include <cstdint>
#include <limits>

// undo ids for the spin box controls, so consecutive edits to one merge
enum class ChangeId : std::uint8_t {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
};

// past every ChangeId, so table switches never merge with a spin box edit
static const auto REPLACE_TABLE_ID =
    static_cast<int>(std::numeric_limits<std::uint8_t>::max()) + 1;
