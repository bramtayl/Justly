#include "models/UnpitchedNotesModel.hpp"

#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "other/Song.hpp"

auto UnpitchedNotesModel::make_empty_row() const -> UnpitchedNote {
  UnpitchedNote unpitched_note;
  unpitched_note.voice_name = song.unpitched_voices.at(0).name;
  return unpitched_note;
}

void UnpitchedNotesModel::add_to_status(
    QTextStream& stream, const int /*row_number*/,
    const UnpitchedNote& unpitched_note) const {
  auto play_state = get_play_state_at_chord(song, parent_chord_number);
  add_timing_to_stream(
      stream, play_state,
      play_state.current_velocity *
          rational_to_double(unpitched_note.velocity_ratio) *
          rational_to_double(
              get_voice(song.unpitched_voices, unpitched_note.voice_name)
                  .velocity_ratio),
      rational_to_double(unpitched_note.beats));
}
