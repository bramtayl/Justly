#include "models/PitchedNotesModel.hpp"

#include "column_numbers/PitchedNoteColumn.hpp"
#include "other/Song.hpp"

auto PitchedNotesModel::make_empty_row() const -> PitchedNote {
  PitchedNote pitched_note;
  pitched_note.voice_name = song.pitched_voices.at(0).name;
  return pitched_note;
}

void PitchedNotesModel::add_to_status(QTextStream& stream,
                                      const int /*row_number*/,
                                      const PitchedNote& pitched_note) const {
  auto play_state = get_play_state_at_chord(song, parent_chord_number);
  add_frequency_to_stream(
      stream,
      play_state.current_key * interval_to_double(pitched_note.interval));
  add_timing_to_stream(stream, play_state,
                       play_state.current_velocity *
                           rational_to_double(pitched_note.velocity_ratio) *
                           rational_to_double(get_voice(song.pitched_voices,
                                                        pitched_note.voice_name)
                                                  .velocity_ratio),
                       rational_to_double(pitched_note.beats));
}
