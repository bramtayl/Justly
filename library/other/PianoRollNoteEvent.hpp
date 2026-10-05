#pragma once

#include "other/Song.hpp"

struct PianoRollNoteEvent {
  double start_time_ms = 0;
  double duration_ms = 0;
  double frequency = 0;  // only meaningful when is_pitched
  int voice_number = 0;
  double velocity = 0;
  int chord_number = 0;
  int note_number = 0;  // index within chord.pitched_notes / .unpitched_notes
  bool is_pitched = true;
};

template <NoteInterface SubNote>
static void append_piano_roll_events(QList<PianoRollNoteEvent>& events,
                                     const PlayState& play_state,
                                     const Song& song, const int chord_number,
                                     const QList<SubNote>& sub_notes) {
  for (auto note_number = 0; note_number < sub_notes.size();
       note_number = note_number + 1) {
    const auto& sub_note = sub_notes.at(note_number);

    PianoRollNoteEvent event;
    event.start_time_ms = play_state.current_time;
    event.duration_ms = get_duration_in_milliseconds(
        play_state.current_tempo, rational_to_double(sub_note.beats));
    event.velocity =
        get_note_velocity(song, sub_note, play_state.current_velocity);
    event.chord_number = chord_number;
    event.note_number = note_number;
    event.voice_number =
        get_voice_number(get_voices<SubNote>(song), sub_note.voice_name);
    event.is_pitched = std::same_as<SubNote, PitchedNote>;
    if constexpr (std::same_as<SubNote, PitchedNote>) {
      event.frequency =
          play_state.current_key * interval_to_double(sub_note.interval);
    }
    events.push_back(event);
  }
}

[[nodiscard]] auto get_piano_roll_events(const Song& song)
    -> QList<PianoRollNoteEvent>;
