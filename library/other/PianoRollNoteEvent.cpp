#include "other/PianoRollNoteEvent.hpp"

#include "other/Song.hpp"
#include "rows/Chord.hpp"

auto get_piano_roll_events(const Song& song) -> QList<PianoRollNoteEvent> {
  QList<PianoRollNoteEvent> events;

  auto play_state = initialize_playstate(song);
  walk_chords(play_state, song.chords, 0, static_cast<int>(song.chords.size()),
              [&events, &play_state, &song](const int chord_number,
                                            const Chord& chord) -> void {
                append_piano_roll_events(events, play_state, song,
                                         chord_number, chord.pitched_notes);
                append_piano_roll_events(events, play_state, song,
                                         chord_number, chord.unpitched_notes);
              });
  return events;
}
