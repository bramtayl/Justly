#pragma once

#include "rows/Chord.hpp"

template <NoteInterface SubNote>
[[nodiscard]] static auto get_notes(Chord& chord) -> QList<SubNote>& {
  if constexpr (std::same_as<SubNote, PitchedNote>) {
    return chord.pitched_notes;
  } else {
    return chord.unpitched_notes;
  }
}

template <NoteInterface SubNote>
[[nodiscard]] static auto get_note(Chord& chord, const int note_number)
    -> SubNote& {
  return get_notes<SubNote>(chord)[note_number];
}

// walks every note across every chord once, calling function(chord_number,
// note_number, voice_name) for each; shared by RenameVoice and
// RemoveVoiceRows to find the notes on the affected voice(s)
template <NoteInterface SubNote, typename Function>
static void for_each_note(QList<Chord>& chords, Function function) {
  for (auto chord_number = 0; chord_number < chords.size();
       chord_number = chord_number + 1) {
    auto& notes = get_notes<SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
         note_number = note_number + 1) {
      function(chord_number, note_number, notes.at(note_number).voice_name);
    }
  }
}

struct NoteLocation {
  int chord_number;
  int note_number;
};

template <NoteInterface SubNote>
static void set_voice_name(QList<Chord>& chords, const NoteLocation& location,
                           const QString& voice_name) {
  get_note<SubNote>(chords[location.chord_number], location.note_number)
      .voice_name = voice_name;
}
