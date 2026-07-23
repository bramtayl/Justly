#pragma once

#include <cstdint>

enum class ChordColumn : std::uint8_t {
  chord_pitched_notes_column,
  chord_unpitched_notes_column,
  chord_interval_column,
  chord_beats_column,
  chord_velocity_ratio_column,
  chord_tempo_ratio_column,
  chord_words_column,
  number_of_chord_columns
};