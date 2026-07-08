#pragma once

#include <QtGui/QUndoStack>
#include <QtWidgets/QMessageBox>

#include "models/VoicesModel.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"

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

// removes a range of voice rows, warning about (and reassigning to the first
// remaining voice) any notes that referenced a removed voice, and shifting
// the voice_number of notes that referenced a later voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RemoveVoiceRows : public QUndoCommand {
  VoicesModel<SubVoice> &voices_model;
  const int first_row_number;
  const QList<SubVoice> old_voice_rows;
  QList<AffectedVoiceNote<SubVoice>> affected_notes;

  RemoveVoiceRows(VoicesModel<SubVoice> &voices_model_input,
                  const int first_row_number_input, const int number_of_rows)
      : voices_model(voices_model_input),
        first_row_number(first_row_number_input),
        old_voice_rows(copy_items(voices_model_input.get_rows(),
                                   first_row_number_input, number_of_rows)) {
    auto &chords = voices_model.song.chords;
    for (auto chord_number = 0; chord_number < chords.size();
         chord_number = chord_number + 1) {
      auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
      for (auto note_number = 0; note_number < notes.size();
           note_number = note_number + 1) {
        const auto old_voice_number = notes.at(note_number).voice_number;
        if (old_voice_number >= first_row_number) {
          affected_notes.push_back(
              {chord_number, note_number, old_voice_number});
        }
      }
    }
  }

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns());
    auto &chords = voices_model.song.chords;
    for (const auto &affected_note : affected_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = affected_note.old_voice_number;
    }
  }

  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
    const auto last_removed_row = first_row_number + number_of_rows - 1;
    auto &chords = voices_model.song.chords;

    auto warned = false;
    for (const auto &affected_note : affected_notes) {
      auto &note = get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number];
      if (affected_note.old_voice_number > last_removed_row) {
        note.voice_number = affected_note.old_voice_number - number_of_rows;
      } else {
        if (!warned) {
          warned = true;
          QString message;
          QTextStream stream(&message);
          stream << QObject::tr("Reassigning voice");
          add_note_location<SubNote>(stream, affected_note.chord_number,
                                     affected_note.note_number);
          stream << QObject::tr(" to the first voice");
          QMessageBox::warning(&voices_model.parent,
                               QObject::tr("Voice removed"), message);
        }
        note.voice_number = 0;
      }
    }
    voices_model.remove_rows(first_row_number, number_of_rows);
  }
};
