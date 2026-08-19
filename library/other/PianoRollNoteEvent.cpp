#include "other/PianoRollNoteEvent.hpp"

#include <utility>

#include "other/Song.hpp"
#include "rows/Chord.hpp"

auto get_piano_roll_events(const Song &song) -> QList<PianoRollNoteEvent> {
  QList<PianoRollNoteEvent> events;

  PlayState play_state;
  initialize_playstate(song, play_state, 0);

  const auto &pitched_voices = song.pitched_voices;
  const auto &unpitched_voices = song.unpitched_voices;
  const auto &chords = song.chords;
  for (auto chord_number = 0; chord_number < chords.size();
       chord_number = chord_number + 1) {
    const auto &chord = chords.at(chord_number);
    modulate(play_state, chord);
    append_piano_roll_events(events, play_state, pitched_voices,
                             unpitched_voices, chord_number,
                             chord.pitched_notes);
    append_piano_roll_events(events, play_state, pitched_voices,
                             unpitched_voices, chord_number,
                             chord.unpitched_notes);
    move_time(play_state, chord);
  }
  return events;
}
