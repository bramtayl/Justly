#pragma once

#include <cstdint>

// undo ids for the spin box controls, so consecutive edits to one merge
enum class ChangeId : std::uint8_t {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id
};
