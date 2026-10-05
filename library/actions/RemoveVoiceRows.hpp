#pragma once

#include <QtGui/QUndoStack>

#include "actions/NoteLocation.hpp"
#include "actions/OrphanedVoiceNameLocation.hpp"

template <VoiceInterface SubVoice, NoteInterface SubNote>
struct VoicesModel;

// removes a range of voice rows, warning about (and reassigning to the first
// remaining voice) any notes that referenced a removed voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RemoveVoiceRows : public QUndoCommand {
  VoicesModel<SubVoice, SubNote>& voices_model;
  const int first_row_number;
  const QList<SubVoice> old_voice_rows;
  QList<OrphanedVoiceNameLocation> orphaned_locations;
  const QString first_voice_name;

  RemoveVoiceRows(VoicesModel<SubVoice, SubNote>& voices_model_input,
                  const int first_row_number_input, const int number_of_rows)
      : voices_model(voices_model_input),
        first_row_number(first_row_number_input),
        old_voice_rows(copy_items(voices_model_input.get_rows(),
                                  first_row_number_input, number_of_rows)),
        // the name of the voice reassigned notes will land on; this action
        // never removes every voice row, so a remaining row always exists to
        // name
        first_voice_name(voices_model.get_rows()
                             .at(first_row_number == 0 ? number_of_rows : 0)
                             .name) {
    for_each_note<SubNote>(voices_model.song.chords,
                           [&](const int chord_number, const int note_number,
                               const QString& voice_name) -> void {
                             if (has_voice(old_voice_rows, voice_name)) {
                               orphaned_locations.push_back(
                                   {.location = {.chord_number = chord_number,
                                                 .note_number = note_number},
                                    .old_voice_name = voice_name});
                             }
                           });
  }

  void undo() override {
    voices_model.insert_rows(first_row_number, old_voice_rows, 0,
                             SubVoice::get_number_of_columns() - 1);
    auto& chords = voices_model.song.chords;
    for (const auto& orphaned_location : orphaned_locations) {
      set_voice_name<SubNote>(chords, orphaned_location.location,
                              orphaned_location.old_voice_name);
    }
  }

  void redo() override {
    auto& chords = voices_model.song.chords;

    // finish every mutation to song.chords and voices_model before showing
    // the warning dialog below -- QMessageBox::warning runs a nested event
    // loop, and anything that repaints while it's up (e.g. the piano roll)
    // must never see a note naming a voice that's already been removed
    for (const auto& orphaned_location : orphaned_locations) {
      set_voice_name<SubNote>(chords, orphaned_location.location,
                              first_voice_name);
    }
    voices_model.remove_rows(first_row_number,
                             static_cast<int>(old_voice_rows.size()));

    if (!orphaned_locations.empty()) {
      warn_reassigned_voices<SubNote>(
          voices_model.parent, static_cast<int>(orphaned_locations.size()),
          first_voice_name);
    }
  }
};
