#pragma once

#include <QtGui/QUndoCommand>

#include "actions/AffectedVoiceNote.hpp"
#include "actions/RenumberedVoiceNote.hpp"

template <VoiceInterface SubVoice>
struct VoicesModel;

// inserts a voice row, shifting the voice_number of any note that referenced
// a voice at or after the insertion point, including any note cells sitting
// on the OS clipboard, so a later paste doesn't land on the wrong voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct InsertVoiceRow : public QUndoCommand {
  VoicesModel<SubVoice>& voices_model;
  const int row_number;
  const SubVoice new_row;
  const QList<RenumberedVoiceNote<SubVoice>> affected_notes;

  InsertVoiceRow(VoicesModel<SubVoice>& voices_model_input,
                 const int row_number_input,
                 SubVoice new_row_input = SubVoice())
      : voices_model(voices_model_input),
        row_number(row_number_input),
        new_row(std::move(new_row_input)),
        // finds every note referencing a voice at or after row_number, so
        // undo/redo can shift its voice_number by a known delta
        affected_notes([&]() -> QList<RenumberedVoiceNote<SubVoice>> {
          QList<RenumberedVoiceNote<SubVoice>> notes;
          for_each_voice_note<SubVoice, SubNote>(
              voices_model.song.chords,
              [&](const int chord_number, const int note_number,
                  const int voice_number) -> void {
                if (voice_number >= row_number) {
                  notes.push_back({chord_number, note_number});
                }
              });
          return notes;
        }()) {}

  void undo() override {
    voices_model.remove_rows(row_number, 1);
    offset_voice_numbers<SubVoice, SubNote>(voices_model.song.chords,
                                            affected_notes, -1);
    renumber_clipboard_voice_numbers<SubNote>(row_number, 1,
                                              /*is_insertion=*/false);
  }

  void redo() override {
    offset_voice_numbers<SubVoice, SubNote>(voices_model.song.chords,
                                            affected_notes, 1);
    renumber_clipboard_voice_numbers<SubNote>(row_number, 1,
                                              /*is_insertion=*/true);
    voices_model.insert_row(row_number, new_row);
  }
};
