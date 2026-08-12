#pragma once

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QUndoStack>

#include "actions/VoiceNoteHelpers.hpp"
#include "other/helpers.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice> struct VoicesModel;

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
  QList<RenumberedVoiceNote<SubVoice>> renumbered_notes;
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
        // the name of the voice reassigned notes will land on; this action
        // never removes every voice row, so a remaining row always exists to
        // name
        first_voice_name(voices_model.get_rows()
                             .at(first_row_number == 0 ? last_removed_row + 1
                                                        : 0)
                             .name) {
    // walks every note once, sorting each one referencing a voice at or
    // after first_row_number into renumbered_notes (voice after the removed
    // range, just shifts down to follow it) or reassigned_notes (voice
    // within the removed range, needs reassigning to the first remaining
    // voice)
    auto &chords = voices_model.song.chords;
    for (auto chord_number = 0; chord_number < chords.size();
        chord_number = chord_number + 1) {
      auto &notes = get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
      for (auto note_number = 0; note_number < notes.size();
          note_number = note_number + 1) {
        const auto voice_number = notes.at(note_number).voice_number;
        if (voice_number < first_row_number) {
          continue;
        }
        if (voice_number > last_removed_row) {
          renumbered_notes.push_back({chord_number, note_number});
        } else {
          reassigned_notes.push_back(
              {chord_number, note_number, voice_number});
        }
      }
    }
  }

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns() - 1);
    auto &chords = voices_model.song.chords;
    offset_voice_numbers<SubVoice, SubNote>(
        chords, renumbered_notes, static_cast<int>(old_voice_rows.size()));
    for (const auto &affected_note : reassigned_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = affected_note.old_voice_number;
    }
    renumber_clipboard_voice_numbers<SubNote>(
        first_row_number, static_cast<int>(old_voice_rows.size()),
        /*is_insertion=*/true);
  }
 
  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
    auto &chords = voices_model.song.chords;

    // finish every mutation to song.chords and voices_model before showing
    // the warning dialog below -- use::warning runs a nested event
    // loop, and anything that repaints while it's up (e.g. the notes table)
    // must never see a note's voice_number pointing at a voice list that
    // hasn't been shrunk to match yet
    offset_voice_numbers<SubVoice, SubNote>(chords, renumbered_notes,
                                            -number_of_rows);
    for (const auto &affected_note : reassigned_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = 0;
    }
    voices_model.remove_rows(first_row_number, number_of_rows);

    if (!reassigned_notes.empty()) {
      warn_reassigned_voices<SubNote>(voices_model.parent,
                                     static_cast<int>(reassigned_notes.size()),
                                     first_voice_name);
    }

    renumber_clipboard_voice_numbers<SubNote>(
        first_row_number, number_of_rows, /*is_insertion=*/false,
        &voices_model.parent, first_voice_name);
  }
};
