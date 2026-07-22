#pragma once

#include <optional>
#include <utility>

#include "other/Song.hpp"

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

// number_of_notes == -1 (default) means "every note in every chord in
// [first_chord_number, first_chord_number + number_of_chords)". A concrete
// number_of_notes restricts to a single chord's note list (number_of_chords
// should be 1 in that case), matching how the Play menu can select either a
// range of chords or a range of notes within one chord.
[[nodiscard]] static auto get_piano_roll_time_bounds(
    const Song &song, const int first_chord_number,
    const int number_of_chords, const int first_note_number = 0,
    const int number_of_notes = -1,
    const std::optional<PianoRollNoteKind> kind_filter = std::nullopt)
    -> std::pair<double, double> {
  const auto baseline_ms =
      get_play_state_at_chord(song, first_chord_number).current_time;

  auto end_ms = baseline_ms;
  const auto single_chord_note_range = number_of_notes != -1;
  for (const auto &event : get_piano_roll_events(song)) {
    if (event.chord_number < first_chord_number ||
        event.chord_number >= first_chord_number + number_of_chords) {
      continue;
    }
    if (single_chord_note_range) {
      if (event.note_number < first_note_number ||
          event.note_number >= first_note_number + number_of_notes) {
        continue;
      }
      if (kind_filter.has_value() && event.kind != *kind_filter) {
        continue;
      }
    }
    end_ms = std::max(end_ms, event.start_time_ms + event.duration_ms);
  }
  return {baseline_ms, end_ms};
}
