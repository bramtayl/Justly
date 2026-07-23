#pragma once

#include <optional>
#include <utility>

#include "other/Song.hpp"
#include "rows/RowType.hpp"

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

// mirrors the filtering in get_piano_roll_time_bounds, but returns every
// matching event's index rather than just the overall time bounds, so the
// piano roll can highlight exactly the notes a table selection corresponds
// to; a chord-row selection matches every note in the selected chords, a
// note-row selection matches only same-kind notes at those row numbers
// within their one parent chord. Voice-row selections (and no selection at
// all, encoded as number_of_rows == 0) have no timeline position and always
// match nothing.
[[nodiscard]] static auto get_selected_piano_roll_event_indices(
    const QList<PianoRollNoteEvent> &events, const RowType selection_row_type,
    const int selection_chord_number, const int selection_first_row_number,
    const int selection_number_of_rows) -> QList<int> {
  QList<int> selected_indices;
  const auto is_chord_selection = selection_row_type == chord_type;
  const auto is_note_selection = selection_row_type == pitched_note_type ||
                                 selection_row_type == unpitched_note_type;
  if (!is_chord_selection && !is_note_selection) {
    return selected_indices;
  }
  const auto kind_filter = selection_row_type == pitched_note_type
                               ? PianoRollNoteKind::pitched_kind
                               : PianoRollNoteKind::unpitched_kind;
  for (auto event_index = 0; event_index < events.size();
      event_index = event_index + 1) {
    const auto &event = events.at(event_index);
    if (is_chord_selection) {
      if (event.chord_number >= selection_first_row_number &&
         event.chord_number <
             selection_first_row_number + selection_number_of_rows) {
        selected_indices.push_back(event_index);
      }
    } else if (event.chord_number == selection_chord_number &&
              event.kind == kind_filter &&
              event.note_number >= selection_first_row_number &&
              event.note_number <
                  selection_first_row_number + selection_number_of_rows) {
      selected_indices.push_back(event_index);
    }
  }
  return selected_indices;
}
