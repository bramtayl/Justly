#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QUndoStack>
#include <QtWidgets/QMessageBox>

#include "actions/VoiceNoteHelpers.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice> struct VoicesModel;

// walks every note once, sorting each one referencing a voice at or after
// first_row_number into renumbered_notes (voice after the removed range,
// just shifts down to follow it) or reassigned_notes (voice within the
// removed range, needs reassigning to the first remaining voice)
template <VoiceInterface SubVoice, NoteInterface SubNote>
static void
split_affected_notes(QList<Chord> &chords, const int first_row_number,
                     const int last_removed_row,
                     QList<AffectedVoiceNote<SubVoice>> &renumbered_notes,
                     QList<AffectedVoiceNote<SubVoice>> &reassigned_notes) {
  for (auto chord_number = 0; chord_number < chords.size();
      chord_number = chord_number + 1) {
    auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
    for (auto note_number = 0; note_number < notes.size();
        note_number = note_number + 1) {
      const auto old_voice_number = notes.at(note_number).voice_number;
      if (old_voice_number < first_row_number) {
        continue;
      }
      const AffectedVoiceNote<SubVoice> affected_note{chord_number,
                                                       note_number,
                                                       old_voice_number};
      if (old_voice_number > last_removed_row) {
        renumbered_notes.push_back(affected_note);
      } else {
        reassigned_notes.push_back(affected_note);
      }
    }
  }
}

// the name of the voice reassigned notes will land on; the caller never
// removes every voice row, so a remaining row always exists to name
template <VoiceInterface SubVoice>
[[nodiscard]] static auto
get_first_remaining_voice_name(VoicesModel<SubVoice> &voices_model,
                               const int first_row_number,
                               const int last_removed_row) -> QString {
  const auto first_remaining_row_number =
      first_row_number == 0 ? last_removed_row + 1 : 0;
  return voices_model.get_rows().at(first_remaining_row_number).name;
}

// removes a range of voice rows, warning about (and reassigning to the first
// remaining voice) any notes that referenced a removed voice, and shifting
// the voice_number of notes that referenced a later voice, including any
// note cells sitting on the OS clipboard, so a later paste doesn't land on
// the wrong voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RemoveVoiceRows : public QUndoCommand {
  VoicesModel<SubVoice> &voices_model;
  const int first_row_number;
  const QList<SubVoice> old_voice_rows;
  const int last_removed_row;
  QList<AffectedVoiceNote<SubVoice>> renumbered_notes;
  QList<AffectedVoiceNote<SubVoice>> reassigned_notes;
  const QString first_voice_name;

  RemoveVoiceRows(VoicesModel<SubVoice> &voices_model_input,
                  const int first_row_number_input, const int number_of_rows)
      : voices_model(voices_model_input),
        first_row_number(first_row_number_input),
        old_voice_rows(copy_items(voices_model_input.get_rows(),
                                   first_row_number_input, number_of_rows)),
        last_removed_row(first_row_number +
                         static_cast<int>(old_voice_rows.size()) - 1),
        first_voice_name(get_first_remaining_voice_name<SubVoice>(
            voices_model, first_row_number, last_removed_row)) {
    split_affected_notes<SubVoice, SubNote>(
        voices_model.song.chords, first_row_number, last_removed_row,
        renumbered_notes, reassigned_notes);
  }

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns());
    restore_affected_notes<SubVoice, SubNote>(voices_model.song.chords,
                                              renumbered_notes);
    restore_affected_notes<SubVoice, SubNote>(voices_model.song.chords,
                                              reassigned_notes);
    renumber_clipboard_voice_numbers<SubNote>(
        first_row_number, static_cast<int>(old_voice_rows.size()),
        /*is_insertion=*/true);
  }

  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
    auto &chords = voices_model.song.chords;

    for (const auto &affected_note : renumbered_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = affected_note.old_voice_number - number_of_rows;
    }

    if (!reassigned_notes.empty()) {
      const auto &first_reassigned_note = reassigned_notes.front();
      const auto number_of_other_reassigned_notes =
          static_cast<int>(reassigned_notes.size()) - 1;
      QString message;
      QTextStream stream(&message);
      stream << QObject::tr("Reassigning voice");
      add_note_location<SubNote>(stream, first_reassigned_note.chord_number,
                                 first_reassigned_note.note_number);
      if (number_of_other_reassigned_notes > 0) {
        stream << QObject::tr(" and ") << number_of_other_reassigned_notes
               << (number_of_other_reassigned_notes == 1
                       ? QObject::tr(" other note")
                       : QObject::tr(" other notes"));
      }
      stream << QObject::tr(" to the first voice \"") << first_voice_name
             << QObject::tr("\"");
      QMessageBox::warning(&voices_model.parent, QObject::tr("Voice removed"),
                           message);
      for (const auto &affected_note : reassigned_notes) {
        get_voice_notes<SubVoice, SubNote>(
            chords[affected_note.chord_number])[affected_note.note_number]
            .voice_number = 0;
      }
    }

    renumber_clipboard_voice_numbers<SubNote>(first_row_number, number_of_rows,
                                              /*is_insertion=*/false);
    voices_model.remove_rows(first_row_number, number_of_rows);
  }
};
