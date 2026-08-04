#pragma once

#include <QtCore/QList>
#include <QtGui/QUndoStack>
#include <utility>

#include "actions/VoiceNoteHelpers.hpp"
#include "rows/Note.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice> struct VoicesModel;

// inserts a voice row, shifting the voice_number of any note that referenced
// a voice at or after the insertion point, including any note cells sitting
// on the OS clipboard, so a later paste doesn't land on the wrong voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct InsertVoiceRow : public QUndoCommand {
  VoicesModel<SubVoice> &voices_model;
  const int row_number;
  const SubVoice new_row;
  const QList<RenumberedVoiceNote<SubVoice>> affected_notes;

  InsertVoiceRow(VoicesModel<SubVoice> &voices_model_input,
                 const int row_number_input,
                 SubVoice new_row_input = SubVoice())
      : voices_model(voices_model_input), row_number(row_number_input),
        new_row(std::move(new_row_input)),
        // finds every note referencing a voice at or after row_number, so
        // undo/redo can shift its voice_number by a known delta
        affected_notes([&]() -> QList<RenumberedVoiceNote<SubVoice>> {
          QList<RenumberedVoiceNote<SubVoice>> notes;
          auto &chords = voices_model.song.chords;
          for (auto chord_number = 0; chord_number < chords.size();
              chord_number = chord_number + 1) {
            auto &chord_notes =
                get_voice_notes<SubVoice, SubNote>(chords[chord_number]);
            for (auto note_number = 0; note_number < chord_notes.size();
                note_number = note_number + 1) {
              if (chord_notes.at(note_number).voice_number >= row_number) {
                notes.push_back({chord_number, note_number});
              }
            }
          }
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
