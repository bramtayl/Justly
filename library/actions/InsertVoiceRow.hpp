#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtGui/QUndoStack>
#include <optional>
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
  const QList<AffectedVoiceNote<SubVoice>> affected_notes;
  std::optional<QByteArray> old_clipboard_bytes;

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
    restore_clipboard_bytes(SubNote::get_cells_mime(), old_clipboard_bytes);
  }

  void redo() override {
    auto &chords = voices_model.song.chords;
    for (const auto &affected_note : affected_notes) {
      get_voice_notes<SubVoice, SubNote>(
          chords[affected_note.chord_number])[affected_note.note_number]
          .voice_number = affected_note.old_voice_number + 1;
    }
    old_clipboard_bytes = renumber_clipboard_voice_numbers<SubNote>(
        [row_number = row_number](const int voice_number) -> int {
          return voice_number >= row_number ? voice_number + 1
                                            : voice_number;
        });
    voices_model.insert_row(row_number, new_row);
  }
};
