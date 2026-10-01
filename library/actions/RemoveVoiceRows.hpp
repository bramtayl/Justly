#pragma once

#include <QtGui/QUndoStack>

#include "actions/OrphanedVoiceNumberLocation.hpp"
#include "actions/ShiftedVoiceNumberLocation.hpp"

template <VoiceInterface SubVoice>
struct VoicesModel;

// removes a range of voice rows, warning about (and reassigning to the first
// remaining voice) any notes that referenced a removed voice, and shifting
// the voice_number of notes that referenced a later voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RemoveVoiceRows : public QUndoCommand {
  VoicesModel<SubVoice>& voices_model;
  const int first_row_number;
  const QList<SubVoice> old_voice_rows;
  const int last_removed_row;
  QList<ShiftedVoiceNumberLocation<SubVoice>> shifted_locations;
  QList<OrphanedVoiceNumberLocation<SubVoice>> orphaned_locations;
  const QString first_voice_name;

  RemoveVoiceRows(VoicesModel<SubVoice>& voices_model_input,
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
        first_voice_name(
            voices_model.get_rows()
                .at(first_row_number == 0 ? last_removed_row + 1 : 0)
                .name) {
    // walks every note once, sorting each one referencing a voice at or
    // after first_row_number into shifted_locations (voice after the removed
    // range, just shifts down to follow it) or orphaned_locations (voice
    // within the removed range, needs reassigning to the first remaining
    // voice)
    for_each_voice_note<SubVoice, SubNote>(
        voices_model.song.chords,
        [&](const int chord_number, const int note_number,
            const int voice_number) -> void {
          if (voice_number < first_row_number) {
            return;
          }
          if (voice_number > last_removed_row) {
            shifted_locations.push_back({chord_number, note_number});
          } else {
            orphaned_locations.push_back(
                {chord_number, note_number, voice_number});
          }
        });
  }

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns() - 1);
    auto& chords = voices_model.song.chords;
    shift_voice_numbers<SubVoice, SubNote>(
        chords, shifted_locations, static_cast<int>(old_voice_rows.size()));
    for (const auto& orphaned_location : orphaned_locations) {
      get_voice_notes<SubVoice, SubNote>(
          chords[orphaned_location.chord_number])[orphaned_location.note_number]
          .voice_number = orphaned_location.old_voice_number;
    }
  }

  void redo() override {
    const auto number_of_rows = static_cast<int>(old_voice_rows.size());
    auto& chords = voices_model.song.chords;

    // finish every mutation to song.chords and voices_model before showing
    // the warning dialog below -- use::warning runs a nested event
    // loop, and anything that repaints while it's up (e.g. the notes table)
    // must never see a note's voice_number pointing at a voice list that
    // hasn't been shrunk to match yet
    shift_voice_numbers<SubVoice, SubNote>(chords, shifted_locations,
                                           -number_of_rows);
    for (const auto& orphaned_location : orphaned_locations) {
      get_voice_notes<SubVoice, SubNote>(
          chords[orphaned_location.chord_number])[orphaned_location.note_number]
          .voice_number = 0;
    }
    voices_model.remove_rows(first_row_number, number_of_rows);

    if (!orphaned_locations.empty()) {
      warn_reassigned_voices<SubNote>(
          voices_model.parent, static_cast<int>(orphaned_locations.size()),
          first_voice_name);
    }
  }
};
