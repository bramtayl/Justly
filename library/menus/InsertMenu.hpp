#pragma once

#include <QtWidgets/QMenu>

#include "actions/InsertRow.hpp"
#include "actions/InsertVoiceRow.hpp"

enum class RowType : std::uint8_t;
struct SongWidget;

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto make_insert_note(RowsModel<SubNote>& notes_model,
                                           const QList<Chord>& chords,
                                           const int row_number)
    -> QUndoCommand* {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return new InsertRow(  // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto make_insert_voice(VoicesModel<SubVoice>& voices_model,
                                            const int row_number)
    -> QUndoCommand* {
  auto& created_voices = voices_model.created_voices;
  created_voices = created_voices + 1;
  SubVoice sub_voice;
  QTextStream stream(&sub_voice.name);
  stream << SubVoice::get_pitched() << " voice " << created_voices;
  return new InsertVoiceRow<  // NOLINT(cppcoreguidelines-owning-memory)
      SubVoice, SubNote>(voices_model, row_number, std::move(sub_voice));
}

void add_insert_row(SongWidget& song_widget, int row_number,
                    RowType new_row_type);

struct InsertMenu : public QMenu {
  QAction insert_after_action;
  QAction insert_into_start_action;

  explicit InsertMenu(SongWidget& song_widget);
};
