#pragma once

#include <cstdint>

enum UnpitchedNoteColumn : std::uint8_t {
  unpitched_note_voice_number_column,
  unpitched_note_beats_column,
  unpitched_note_velocity_ratio_column,
  unpitched_note_words_column,
  number_of_unpitched_note_columns
};