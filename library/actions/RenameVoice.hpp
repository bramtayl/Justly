#pragma once

#include <QtGui/QUndoCommand>

#include "actions/NoteLocation.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice, NoteInterface SubNote>
struct VoicesModel;

template <VoiceInterface SubVoice, NoteInterface SubNote>
void set_notes_voice_name(VoicesModel<SubVoice, SubNote>& voices_model,
                          const int row_number,
                          const QList<NoteLocation>& note_locations,
                          const QString& name) {
  emit voices_model.show_rows_requested(-1);
  auto& chords = voices_model.song.chords;
  for (const auto& note_location : note_locations) {
    set_voice_name<SubNote>(chords, note_location, name);
  }
  voices_model.set_cell(row_number, SubVoice::get_name_column(), name);
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RenameVoice : public QUndoCommand {
  VoicesModel<SubVoice, SubNote>& voices_model;
  const int row_number;
  const QString old_name;
  const QString new_name;
  QList<NoteLocation> note_locations;

  RenameVoice(VoicesModel<SubVoice, SubNote>& voices_model_input,
              const int row_number_input, QString new_name_input)
      : voices_model(voices_model_input),
        row_number(row_number_input),
        old_name(voices_model.get_rows().at(row_number).name),
        new_name(std::move(new_name_input)) {
    for_each_note<SubNote>(
        voices_model.song.chords,
        [&](const int chord_number, const int note_number,
            const QString& voice_name) -> void {
          if (voice_name == old_name) {
            note_locations.push_back(
                {.chord_number = chord_number, .note_number = note_number});
          }
        });
  }

  void undo() override {
    set_notes_voice_name(voices_model, row_number, note_locations, old_name);
  }

  void redo() override {
    set_notes_voice_name(voices_model, row_number, note_locations, new_name);
  }
};
