#include "menus/EditMenu.hpp"

void add_delete_cells(SongWidget &song_widget) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_table = song_widget.switch_column.switch_table;

  const auto &range = get_only_range(switch_table);

  undo_stack.push(
      dispatch_row_type(switch_table, [&range](auto &rows_model) -> QUndoCommand * {
        return new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
            rows_model, range);
      }));
}

void copy_selection(const SwitchTable &switch_table) {
  const auto &range = get_only_range(switch_table);
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  dispatch_row_type(switch_table,
                    [&mime_data, &range](const auto &rows_model) -> void {
                      copy_from_model(mime_data, rows_model, range);
                    });
  get_clipboard().setMimeData(&mime_data);
}

EditMenu::EditMenu(SongWidget &song_widget)
    : QMenu(EditMenu::tr("&Edit")), paste_menu(PasteMenu(song_widget)),
      insert_menu(InsertMenu(song_widget)) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_table = song_widget.switch_column.switch_table;

  auto &undo_action = get_reference(undo_stack.createUndoAction(this));
  undo_action.setShortcuts(QKeySequence::Undo);

  auto &redo_action = get_reference(undo_stack.createRedoAction(this));
  redo_action.setShortcuts(QKeySequence::Redo);

  addAction(&undo_action);
  addAction(&redo_action);
  addSeparator();

  add_menu_action(*this, cut_action, QKeySequence::Cut);
  add_menu_action(*this, copy_action, QKeySequence::Copy);
  addMenu(&paste_menu);
  addSeparator();

  addMenu(&insert_menu);
  add_menu_action(*this, delete_cells_action, QKeySequence::Delete, false);
  add_menu_action(*this, remove_rows_action, QKeySequence::DeleteStartOfWord,
                  false);
  addSeparator();

  QObject::connect(&cut_action, &QAction::triggered, this, [&song_widget]() -> auto {
    copy_selection(song_widget.switch_column.switch_table);
    add_delete_cells(song_widget);
  });

  QObject::connect(&copy_action, &QAction::triggered, &switch_table,
                   [&switch_table]() -> auto { copy_selection(switch_table); });

  QObject::connect(&delete_cells_action, &QAction::triggered, this,
                   [&song_widget]() -> auto { add_delete_cells(song_widget); });

  QObject::connect(
      &remove_rows_action, &QAction::triggered, this, [&song_widget]() -> auto {
        auto &switch_table = song_widget.switch_column.switch_table;
        auto &undo_stack = song_widget.undo_stack;

        const auto &range = get_only_range(switch_table);
        const auto first_row_number = range.top();
        const auto number_of_rows = get_number_of_rows(range);

        // remove_rows_action is disabled (see update_actions) whenever the
        // selection covers every remaining voice row, so the voice cases
        // below never need to guard against removing the last one
        QUndoCommand *undo_command = nullptr;
        switch (switch_table.delegate.current_row_type) {
        case RowType::chord_type:
          undo_command = make_remove_command(
              switch_table.chords_model, first_row_number, number_of_rows);
          break;
        case RowType::pitched_note_type:
          undo_command =
              make_remove_command(switch_table.pitched_notes_model,
                                  first_row_number, number_of_rows);
          break;
        case RowType::unpitched_note_type:
          undo_command =
              make_remove_command(switch_table.unpitched_notes_model,
                                  first_row_number, number_of_rows);
          break;
        case RowType::pitched_voice_type:
          undo_command = new RemoveVoiceRows< // NOLINT(cppcoreguidelines-owning-memory)
              PitchedVoice, PitchedNote>(switch_table.pitched_voices_model,
                                         first_row_number, number_of_rows);
          break;
        case RowType::unpitched_voice_type:
          undo_command = new RemoveVoiceRows< // NOLINT(cppcoreguidelines-owning-memory)
              UnpitchedVoice, UnpitchedNote>(
              switch_table.unpitched_voices_model, first_row_number,
              number_of_rows);
          break;
        }
        undo_stack.push(undo_command);
      });
}
