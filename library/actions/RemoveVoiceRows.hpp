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

// how many affected_notes fall within the removed range, i.e. need
// reassigning to the first remaining voice rather than just shifting down
template <VoiceInterface SubVoice>
[[nodiscard]] static auto
count_reassigned_notes(const QList<AffectedVoiceNote<SubVoice>> &affected_notes,
                       const int last_removed_row) -> int {
  auto count = 0;
  for (const auto &affected_note : affected_notes) {
    if (affected_note.old_voice_number <= last_removed_row) {
      count = count + 1;
    }
  }
  return count;
}

// the name of the voice reassigned notes will land on, or empty if nothing
// needs reassigning; only valid to look up when at least one note is
// reassigned, since removing every voice row leaves no remaining row
template <VoiceInterface SubVoice>
[[nodiscard]] static auto
get_first_remaining_voice_name(VoicesModel<SubVoice> &voices_model,
                               const int first_row_number,
                               const int last_removed_row,
                               const int number_of_reassigned_notes)
    -> QString {
  if (number_of_reassigned_notes == 0) {
    return {};
  }
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
  const QList<AffectedVoiceNote<SubVoice>> affected_notes;
  const int last_removed_row;
  const int number_of_reassigned_notes;
  const QString first_voice_name;

  RemoveVoiceRows(VoicesModel<SubVoice> &voices_model_input,
                  const int first_row_number_input, const int number_of_rows)
      : voices_model(voices_model_input),
        first_row_number(first_row_number_input),
        old_voice_rows(copy_items(voices_model_input.get_rows(),
                                   first_row_number_input, number_of_rows)),
        affected_notes(find_affected_notes<SubVoice, SubNote>(
            voices_model.song.chords, first_row_number)),
        last_removed_row(first_row_number +
                         static_cast<int>(old_voice_rows.size()) - 1),
        number_of_reassigned_notes(
            count_reassigned_notes<SubVoice>(affected_notes, last_removed_row)),
        first_voice_name(get_first_remaining_voice_name<SubVoice>(
            voices_model, first_row_number, last_removed_row,
            number_of_reassigned_notes)) {}

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns());
    restore_affected_notes<SubVoice, SubNote>(voices_model.song.chords,
                                              affected_notes);
    renumber_clipboard_voice_numbers<SubNote>(make_insert_voice_transform(
        first_row_number, static_cast<int>(old_voice_rows.size())));
  }

  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
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
    renumber_clipboard_voice_numbers<SubNote>(
        make_remove_voice_transform(first_row_number, number_of_rows));
    voices_model.remove_rows(first_row_number, number_of_rows);
  }
};
