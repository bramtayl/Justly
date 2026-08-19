#pragma once

#include <QtCore/QList>
#include <concepts>

#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

struct PitchedVoice;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto get_voice_notes(Chord& chord) -> QList<SubNote>& {
  if constexpr (std::same_as<SubVoice, PitchedVoice>) {
    return chord.pitched_notes;
  } else {
    return chord.unpitched_notes;
  }
}

// walks every note across every chord once, calling function(chord_number,
// note_number, voice_number) for each; shared by InsertVoiceRow and
// RemoveVoiceRows, which otherwise each hand-roll the same nested loop to
// sort notes by how their voice_number relates to the affected row range
template <VoiceInterface SubVoice, NoteInterface SubNote, typename Function>
static void for_each_voice_note(QList<Chord>& chords, Function function) {
  for (auto chord_number = 0; chord_number < chords.size();
       chord_number = chord_number + 1) {
    auto& notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
         note_number = note_number + 1) {
      function(chord_number, note_number, notes.at(note_number).voice_number);
    }
  }
}

// a note whose voice_number was (or will be) shifted by a fixed delta, so the
// pre-shift voice_number can always be recovered as current - delta; used
// where that delta is known at both redo and undo, so there's no need to
// separately store the old voice number
template <VoiceInterface SubVoice>
struct RenumberedVoiceNote {
  int chord_number;
  int note_number;
};

template <VoiceInterface SubVoice, NoteInterface SubNote>
static void offset_voice_numbers(
    QList<Chord>& chords,
    const QList<RenumberedVoiceNote<SubVoice>>& affected_notes,
    const int delta) {
  for (const auto& affected_note : affected_notes) {
    get_voice_notes<SubVoice, SubNote>(
        chords[affected_note.chord_number])[affected_note.note_number]
        .voice_number += delta;
  }
}
