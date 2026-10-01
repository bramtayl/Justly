#pragma once

#include <QtCore/QModelIndex>
#include <QtGui/QUndoCommand>

#include "actions/NoteLocation.hpp"
#include "rows/Voice.hpp"

template <VoiceInterface SubVoice>
struct VoicesModel;

// renames a voice, and every note naming it, so notes follow their voice
template <VoiceInterface SubVoice, NoteInterface SubNote>
struct RenameVoice : public QUndoCommand {
  VoicesModel<SubVoice>& voices_model;
  QModelIndex index;
  const QString old_name;
  const QString new_name;
  QList<NoteLocation> note_locations;

  RenameVoice(VoicesModel<SubVoice>& voices_model_input,
              const QModelIndex& index_input, QString new_name_input)
      : voices_model(voices_model_input),
        index(index_input),
        old_name(voices_model.get_rows().at(index.row()).name),
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

  void set_name(const QString& name) {
    auto& chords = voices_model.song.chords;
    for (const auto& note_location : note_locations) {
      get_note<SubNote>(chords[note_location.chord_number],
                        note_location.note_number)
          .voice_name = name;
    }
    voices_model.set_cell(index, name);
  }

  void undo() override { set_name(old_name); }

  void redo() override { set_name(new_name); }
};
