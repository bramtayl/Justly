#include "models/ChordsModel.hpp"

#include "other/Song.hpp"

void ChordsModel::add_to_status(QTextStream& stream, const int row_number,
                                const Chord& chord) const {
  auto play_state = get_play_state_at_chord(song, row_number);
  add_frequency_to_stream(stream, play_state.current_key);
  add_timing_to_stream(stream, play_state, play_state.current_velocity,
                       rational_to_double(chord.beats));
}
