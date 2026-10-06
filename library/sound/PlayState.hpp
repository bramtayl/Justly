#pragma once

struct PlayState {
  double current_time = 0;

  double current_key = 0;
  double current_velocity = 0;
  double current_tempo = 0;
};

[[nodiscard]] inline auto get_duration_in_milliseconds(
    const double beats_per_minute, const double beats_double) -> double {
  static const auto MILLISECONDS_PER_MINUTE = 60000;
  return beats_double * MILLISECONDS_PER_MINUTE / beats_per_minute;
}
