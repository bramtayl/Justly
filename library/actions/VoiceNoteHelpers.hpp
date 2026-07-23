#pragma once

#include <QtCore/QList>
#include <concepts>

#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

struct PitchedVoice;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto get_voice_notes(Chord &chord) -> QList<SubNote> & {
  if constexpr (std::same_as<SubVoice, PitchedVoice>) {
    return chord.pitched_notes;
  } else {
    return chord.unpitched_notes;
  }
}

template <VoiceInterface SubVoice> struct AffectedVoiceNote {
  int chord_number;
  int note_number;
  int old_voice_number;
};

// finds every note referencing a voice at or after first_affected_voice_number,
// so InsertVoiceRow/RemoveVoiceRows can shift or restore voice_number on
// undo/redo
template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
find_affected_notes(QList<Chord> &chords,
                    const int first_affected_voice_number)
    -> QList<AffectedVoiceNote<SubVoice>> {
  QList<AffectedVoiceNote<SubVoice>> affected_notes;
  for (auto chord_number = 0; chord_number < chords.size();
      chord_number = chord_number + 1) {
    auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
        note_number = note_number + 1) {
      const auto old_voice_number = notes.at(note_number).voice_number;
      if (old_voice_number >= first_affected_voice_number) {
        affected_notes.push_back(
            {chord_number, note_number, old_voice_number});
      }
    }
  }
  return affected_notes;
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
static void restore_affected_notes(
    QList<Chord> &chords,
    const QList<AffectedVoiceNote<SubVoice>> &affected_notes) {
  for (const auto &affected_note : affected_notes) {
    get_voice_notes<SubVoice, SubNote>(
        chords[affected_note.chord_number])[affected_note.note_number]
        .voice_number = affected_note.old_voice_number;
  }
}
