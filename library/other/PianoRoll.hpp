#pragma once

#include <QtCore/QList>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <algorithm>
#include <cstdint>
#include <type_traits>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "other/Song.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Row.hpp"
#include "sound/PlayState.hpp"

struct PitchedNote;
struct PitchedVoice;
struct UnpitchedNote;
struct UnpitchedVoice;

enum class PianoRollNoteKind : std::uint8_t { pitched_kind, unpitched_kind };

struct PianoRollNoteEvent {
  double start_time_ms = 0;
  double duration_ms = 0;
  double frequency = 0; // only meaningful when kind == pitched_kind
  int voice_number = 0;
  double velocity = 0;
  int chord_number = 0;
  int note_number = 0; // index within chord.pitched_notes / .unpitched_notes
  PianoRollNoteKind kind = PianoRollNoteKind::pitched_kind;
};

template <NoteInterface SubNote>
static void
append_piano_roll_events(QList<PianoRollNoteEvent> &events,
                         const PlayState &play_state,
                         const QList<PitchedVoice> &pitched_voices,
                         const QList<UnpitchedVoice> &unpitched_voices,
                         const int chord_number,
                         const QList<SubNote> &sub_notes) {
  for (auto note_number = 0; note_number < sub_notes.size();
       note_number = note_number + 1) {
    const auto &sub_note = sub_notes.at(note_number);
    const auto &voice_velocity_ratio =
        sub_note.get_voice_velocity_ratio(pitched_voices, unpitched_voices);

    PianoRollNoteEvent event;
    event.start_time_ms = play_state.current_time;
    event.duration_ms = get_duration_in_milliseconds(
        play_state.current_tempo, rational_to_double(sub_note.beats));
    event.velocity = play_state.current_velocity *
                     rational_to_double(sub_note.velocity_ratio) *
                     rational_to_double(voice_velocity_ratio);
    event.voice_number = sub_note.voice_number;
    event.chord_number = chord_number;
    event.note_number = note_number;
    if constexpr (std::is_same_v<SubNote, PitchedNote>) {
      event.kind = PianoRollNoteKind::pitched_kind;
      event.frequency =
          play_state.current_key * interval_to_double(sub_note.interval);
    } else {
      event.kind = PianoRollNoteKind::unpitched_kind;
    }
    events.push_back(event);
  }
}

[[nodiscard]] static inline auto get_piano_roll_events(const Song &song)
    -> QList<PianoRollNoteEvent> {
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

// each chord's start time, in chord order -- chords are laid out back-to-
// back with no gaps, so a chord's own end time is simply the next chord's
// start (or, for the last chord, whatever the caller already knows the
// song's end time to be)
[[nodiscard]] static inline auto get_chord_start_times(const Song &song)
    -> QList<double> {
  QList<double> start_times;

  PlayState play_state;
  initialize_playstate(song, play_state, 0);

  for (const auto &chord : song.chords) {
    modulate(play_state, chord);
    start_times.push_back(play_state.current_time);
    move_time(play_state, chord);
  }
  return start_times;
}

// which chord's time range (as laid out by get_chord_start_times) contains
// time_ms -- the last chord whose start is at or before time_ms, or -1 if
// there are no chords yet or time_ms falls before the first one
[[nodiscard]] static inline auto
get_chord_number_at_time(const QList<double> &chord_start_times,
                         const double time_ms) -> int {
  const auto first_later_iterator =
      std::ranges::upper_bound(chord_start_times, time_ms);
  return static_cast<int>(first_later_iterator - chord_start_times.begin()) -
         1;
}
