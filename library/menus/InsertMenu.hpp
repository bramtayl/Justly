#pragma once

#include <QtWidgets/QMenu>

#include "actions/InsertRemoveRows.hpp"
#include "models/NotesModel.hpp"
#include "models/VoicesModel.hpp"
#include "rows/Chord.hpp"

struct WindowBody;

template <RowInterface SubRow>
[[nodiscard]] auto make_insert_row(RowsModel<SubRow>& rows_model,
                                   const int row_number, SubRow new_row)
    -> QUndoCommand* {
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, row_number, QList<SubRow>({std::move(new_row)}), 0,
      SubRow::get_number_of_columns() - 1, false);
}

// inserts a new row; overloaded below for notes and voices, which need more
// than make_empty_row
template <RowInterface SubRow>
[[nodiscard]] auto make_insert_command(RowsModel<SubRow>& rows_model,
                                       const int row_number) -> QUndoCommand* {
  return make_insert_row(rows_model, row_number, rows_model.make_empty_row());
}

template <NoteInterface SubNote>
[[nodiscard]] auto make_insert_command(NotesModel<SubNote>& notes_model,
                                       const int row_number) -> QUndoCommand* {
  auto sub_note = notes_model.make_empty_row();
  sub_note.beats =
      notes_model.song.chords.at(notes_model.parent_chord_number).beats;
  return make_insert_row(notes_model, row_number, std::move(sub_note));
}

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] auto make_insert_command(
    VoicesModel<SubVoice, SubNote>& voices_model, const int row_number)
    -> QUndoCommand* {
  auto& created_voices = voices_model.created_voices;
  const auto& voices = voices_model.get_rows();
  SubVoice sub_voice;
  // skip names already taken, e.g. by a voice loaded from a file
  do {
    ++created_voices;
    sub_voice.name =
        QString("%1 voice %2").arg(SubVoice::get_pitched()).arg(created_voices);
  } while (get_named_index(voices, sub_voice.name) != voices.cend());
  return make_insert_row(voices_model, row_number, std::move(sub_voice));
}

// inserts a new row into the table currently shown
void add_insert_row(WindowBody& window_body, int row_number);

struct InsertMenu : public QMenu {
  QAction insert_after_action;
  QAction insert_into_start_action;

  explicit InsertMenu(WindowBody& window_body);
};
