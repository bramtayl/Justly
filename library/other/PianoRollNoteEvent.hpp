#pragma once

#include <QtCore/QList>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
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

struct PianoRollNoteEvent {
  double start_time_ms = 0;
  double duration_ms = 0;
  double frequency = 0; // only meaningful when is_pitched
  int voice_number = 0;
  double velocity = 0;
  int chord_number = 0;
  int note_number = 0; // index within chord.pitched_notes / .unpitched_notes
  bool is_pitched = true;
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
      event.is_pitched = true;
      event.frequency =
          play_state.current_key * interval_to_double(sub_note.interval);
    } else {
      event.is_pitched = false;
    }
    events.push_back(event);
  }
}

[[nodiscard]] auto get_piano_roll_events(const Song &song)
    -> QList<PianoRollNoteEvent>;
