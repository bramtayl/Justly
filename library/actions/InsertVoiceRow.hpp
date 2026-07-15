#pragma once

#include <QtGui/QUndoStack>

#include "actions/VoiceNoteHelpers.hpp"
#include "models/VoicesModel.hpp"
#include "rows/Chord.hpp"

// inserts a voice row, shifting the voice_number of any note that referenced
// a voice at or after the insertion point
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct InsertVoiceRow : public QUndoCommand {
  VoicesModel<SubVoice> &voices_model;
  const int row_number;
  const SubVoice new_row;
  const QList<AffectedVoiceNote<SubVoice>> affected_notes;

  InsertVoiceRow(VoicesModel<SubVoice> &voices_model_input,
                 const int row_number_input,
                 SubVoice new_row_input = SubVoice())
      : voices_model(voices_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)),
        affected_notes(find_affected_notes<SubVoice, SubNote>(
            voices_model.song.chords, row_number)) {}

  void undo() override {
    voices_model.remove_rows(row_number, 1);
    restore_affected_notes<SubVoice, SubNote>(voices_model.song.chords,
                                              affected_notes);
  }

  void redo() override {
    auto &chords = voices_model.song.chords;
    for (const auto &affected_note : affected_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = affected_note.old_voice_number + 1;
    }
    voices_model.insert_row(row_number, new_row);
  }
};
