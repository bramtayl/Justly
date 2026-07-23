#pragma once

#include <cstdint>

enum class ChangeId : std::uint8_t {
  gain_id,
  starting_key_id,
  starting_velocity_id,
  starting_tempo_id,
  replace_table_id
};