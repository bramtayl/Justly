#pragma once

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtGui/QUndoStack>
#include <QtWidgets/QMessageBox>

#include "actions/VoiceNoteHelpers.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice> struct VoicesModel;

// removes a range of voice rows, warning about (and reassigning to the first
// remaining voice) any notes that referenced a removed voice, and shifting
// the voice_number of notes that referenced a later voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RemoveVoiceRows : public QUndoCommand {
  VoicesModel<SubVoice> &voices_model;
  const int first_row_number;
  const QList<SubVoice> old_voice_rows;
  const QList<AffectedVoiceNote<SubVoice>> affected_notes;

  RemoveVoiceRows(VoicesModel<SubVoice> &voices_model_input,
                  const int first_row_number_input, const int number_of_rows)
      : voices_model(voices_model_input),
        first_row_number(first_row_number_input),
        old_voice_rows(copy_items(voices_model_input.get_rows(),
                                   first_row_number_input, number_of_rows)),
        affected_notes(find_affected_notes<SubVoice, SubNote>(
            voices_model.song.chords, first_row_number)) {}

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns());
    restore_affected_notes<SubVoice, SubNote>(voices_model.song.chords,
                                              affected_notes);
  }

  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
    const auto last_removed_row = first_row_number + number_of_rows - 1;
    auto &chords = voices_model.song.chords;

    auto number_of_reassigned_notes = 0;
    for (const auto &affected_note : affected_notes) {
      if (affected_note.old_voice_number <= last_removed_row) {
        number_of_reassigned_notes = number_of_reassigned_notes + 1;
      }
    }

    auto warned = false;
    for (const auto &affected_note : affected_notes) {
      auto &note = get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number];
      if (affected_note.old_voice_number > last_removed_row) {
        note.voice_number = affected_note.old_voice_number - number_of_rows;
      } else {
        if (!warned) {
          warned = true;
          const auto &rows = voices_model.get_rows();
          const auto first_remaining_row_number =
              first_row_number == 0 ? last_removed_row + 1 : 0;
          const auto &first_voice_name =
              rows.at(first_remaining_row_number).name;
          const auto number_of_other_reassigned_notes =
              number_of_reassigned_notes - 1;
          QString message;
          QTextStream stream(&message);
          stream << QObject::tr("Reassigning voice");
          add_note_location<SubNote>(stream, affected_note.chord_number,
                                     affected_note.note_number);
          if (number_of_other_reassigned_notes > 0) {
            stream << QObject::tr(" and ") << number_of_other_reassigned_notes
                   << (number_of_other_reassigned_notes == 1
                           ? QObject::tr(" other note")
                           : QObject::tr(" other notes"));
          }
          stream << QObject::tr(" to the first voice \"") << first_voice_name
                 << QObject::tr("\"");
          QMessageBox::warning(&voices_model.parent,
                               QObject::tr("Voice removed"), message);
        }
        note.voice_number = 0;
      }
    }
    voices_model.remove_rows(first_row_number, number_of_rows);
  }
};
