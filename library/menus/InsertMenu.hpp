#pragma once

#include <QtWidgets/QMenu>

#include "actions/InsertRemoveRows.hpp"
#include "models/VoicesModel.hpp"
#include "rows/Chord.hpp"

enum class RowType : std::uint8_t;
struct WindowBody;

template <RowInterface SubRow>
[[nodiscard]] static auto make_insert_row(RowsModel<SubRow>& rows_model,
                                          const int row_number,
                                          SubRow new_row = SubRow())
    -> QUndoCommand* {
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, QList<SubRow>({std::move(new_row)}), 0,
      SubRow::get_number_of_columns() - 1, false);
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto make_insert_note(RowsModel<SubNote>& notes_model,
                                           const QList<Chord>& chords,
                                           const int row_number)
    -> QUndoCommand* {
  auto sub_note = notes_model.make_empty_row();
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return make_insert_row(notes_model, row_number, std::move(sub_note));
}

template <VoiceInterface SubVoice>
[[nodiscard]] static auto make_insert_voice(VoicesModel<SubVoice>& voices_model,
                                            const int row_number)
    -> QUndoCommand* {
  auto& created_voices = voices_model.created_voices;
  const auto& voices = voices_model.get_rows();
  SubVoice sub_voice;
  // skip names already taken, e.g. by a voice loaded from a file
  do {
    created_voices = created_voices + 1;
    sub_voice.name =
        QString("%1 voice %2").arg(SubVoice::get_pitched()).arg(created_voices);
  } while (get_named_index(voices, sub_voice.name) != voices.cend());
  return make_insert_row(voices_model, row_number, std::move(sub_voice));
}

void add_insert_row(WindowBody& window_body, int row_number,
                    RowType new_row_type);

struct InsertMenu : public QMenu {
  QAction insert_after_action;
  QAction insert_into_start_action;

  explicit InsertMenu(WindowBody& window_body);
};
