#pragma once

#include <QtWidgets/QMenu>

#include "actions/InsertRow.hpp"
#include "rows/Voice.hpp"
#include "widgets/SongWidget.hpp"

template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] static auto
make_insert_note(const QList<SubVoice> &voices, RowsModel<SubNote> &notes_model,
                 const QList<Chord> &chords,
                 const int row_number) -> QUndoCommand * {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  sub_note.voice = get_first(voices).name;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

template <VoiceInterface SubVoice>
[[nodiscard]] static auto
make_insert_voice(RowsModel<SubVoice> &voices_model,
                  const int row_number) -> QUndoCommand * {
  SubVoice sub_voice;
  QTextStream stream(&sub_voice.name);
  stream << SubVoice::get_description() << " " << row_number + 1;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      voices_model, row_number, std::move(sub_voice));
}

static void add_insert_row(SongWidget &song_widget, const int row_number,
                           const RowType new_row_type) {
  auto &switch_table = song_widget.switch_column.switch_table;
  QUndoCommand *undo_command = nullptr;
  const auto &chords = song_widget.song.chords;
  switch (new_row_type) {
  case chord_type:
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, row_number);
    break;
  case pitched_note_type:
    undo_command =
        make_insert_note(song_widget.song.pitched_voices,
                         switch_table.pitched_notes_model, chords, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_insert_note(song_widget.song.unpitched_voices,
                                    switch_table.unpitched_notes_model, chords,
                                    row_number);
    break;
  case pitched_voice_type:
    undo_command =
        make_insert_voice(switch_table.pitched_voices_model, row_number);
    break;
  case unpitched_voice_type:
    undo_command =
        make_insert_voice(switch_table.unpitched_voices_model, row_number);
    break;
  default:
    Q_ASSERT(false);
  }
  song_widget.undo_stack.push(undo_command);
}

static void add_insert_row_default(SongWidget &song_widget,
                                   const int row_number) {
  add_insert_row(
      song_widget, row_number,
      song_widget.switch_column.switch_table.delegate.current_row_type);
}

struct InsertMenu : public QMenu {
  QAction insert_after_action = QAction(InsertMenu::tr("&After"));
  QAction insert_into_start_action = QAction(InsertMenu::tr("&Into start"));

  explicit InsertMenu(SongWidget &song_widget)
      : QMenu(InsertMenu::tr("&Insert row")) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_start_action, QKeySequence::AddTab);

    QObject::connect(
        &insert_after_action, &QAction::triggered, this, [&song_widget]() {
          add_insert_row_default(song_widget, get_next_row(song_widget));
        });

    QObject::connect(
        &insert_into_start_action, &QAction::triggered, this,
        [&song_widget]() { add_insert_row_default(song_widget, 0); });
  }
};