#pragma once

#include <QtWidgets/QMenu>

#include "actions/InsertRow.hpp"
#include "widgets/SongWidget.hpp"

template <NoteInterface SubNote>
[[nodiscard]] static auto
make_insert_note(NotesModel<SubNote> &notes_model, const QList<Chord> &chords,
                 const int row_number) -> QUndoCommand * {
  SubNote sub_note;
  sub_note.beats = chords[notes_model.parent_chord_number].beats;
  return new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
      notes_model, row_number, std::move(sub_note));
}

static void add_insert_row(SongWidget &song_widget, const int row_number) {
  auto &switch_table = song_widget.switch_column.switch_table;
  const auto current_row_type = switch_table.current_row_type;
  QUndoCommand *undo_command = nullptr;
  const auto &chords = song_widget.song.chords;
  switch (current_row_type) {
  case chord_type:
    undo_command = new InsertRow( // NOLINT(cppcoreguidelines-owning-memory)
        switch_table.chords_model, row_number);
    break;
  case pitched_note_type:
    undo_command =
        make_insert_note(switch_table.pitched_notes_model, chords, row_number);
    break;
  case unpitched_note_type:
    undo_command = make_insert_note(switch_table.unpitched_notes_model, chords,
                                    row_number);
    break;
  default:
    Q_ASSERT(false);
  }
  song_widget.undo_stack.push(undo_command);
}

struct InsertMenu : public QMenu {
  QAction insert_after_action = QAction(InsertMenu::tr("&After"));
  QAction insert_into_start_action = QAction(InsertMenu::tr("&Into start"));

  explicit InsertMenu(SongWidget &song_widget)
      : QMenu(InsertMenu::tr("&Insert")) {
    add_menu_action(*this, insert_after_action,
                    QKeySequence::InsertLineSeparator, false);
    add_menu_action(*this, insert_into_start_action, QKeySequence::AddTab);

    QObject::connect(&insert_after_action, &QAction::triggered, this,
                     [&song_widget]() {
                       add_insert_row(song_widget, get_next_row(song_widget));
                     });

    QObject::connect(&insert_into_start_action, &QAction::triggered, this,
                     [&song_widget]() { add_insert_row(song_widget, 0); });
  }
};