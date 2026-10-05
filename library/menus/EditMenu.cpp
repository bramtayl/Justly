#include "menus/EditMenu.hpp"

#include "actions/RemoveVoiceRows.hpp"
#include "actions/SetCells.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

namespace {

void add_delete_cells(WindowBody& window_body) {
  auto& undo_stack = window_body.undo_stack;
  auto& switch_table = window_body.switch_column.switch_table;

  const auto& range = get_only_range(switch_table);

  undo_stack.push(dispatch_row_type(
      switch_table, [&range](auto& rows_model) -> QUndoCommand* {
        const auto number_of_rows = get_number_of_rows(range);
        auto empty_row = rows_model.make_empty_row();
        return new SetCells(  // NOLINT(cppcoreguidelines-owning-memory)
            rows_model, range.top(), number_of_rows, range.left(),
            range.right(), QList<decltype(empty_row)>(number_of_rows, empty_row));
      }));
}

void copy_selection(const SwitchTable& switch_table) {
  const auto& range = get_only_range(switch_table);
  auto& mime_data =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  dispatch_row_type(switch_table,
                    [&mime_data, &range](const auto& rows_model) -> void {
                      copy_from_model(mime_data, rows_model, range);
                    });
  get_clipboard().setMimeData(&mime_data);
}

}  // namespace

EditMenu::EditMenu(WindowBody& window_body)
    : QMenu(EditMenu::tr("&Edit")),
      cut_action(EditMenu::tr("&Cut")),
      copy_action(EditMenu::tr("&Copy")),
      paste_menu(window_body),
      insert_menu(window_body),
      delete_cells_action(EditMenu::tr("&Delete cells")),
      remove_rows_action(EditMenu::tr("&Remove rows")) {
  auto& undo_stack = window_body.undo_stack;
  auto& switch_table = window_body.switch_column.switch_table;

  auto& undo_action = get_reference(undo_stack.createUndoAction(this));
  undo_action.setShortcuts(QKeySequence::Undo);

  auto& redo_action = get_reference(undo_stack.createRedoAction(this));
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

  QObject::connect(&cut_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     copy_selection(window_body.switch_column.switch_table);
                     add_delete_cells(window_body);
                   });

  QObject::connect(&copy_action, &QAction::triggered, &switch_table,
                   [&switch_table]() -> auto { copy_selection(switch_table); });

  QObject::connect(&delete_cells_action, &QAction::triggered, this,
                   [&window_body]() -> auto { add_delete_cells(window_body); });

  QObject::connect(
      &remove_rows_action, &QAction::triggered, this, [&window_body]() -> auto {
        auto& switch_table = window_body.switch_column.switch_table;
        auto& undo_stack = window_body.undo_stack;

        const auto& range = get_only_range(switch_table);
        const auto first_row_number = range.top();
        const auto number_of_rows = get_number_of_rows(range);

        // remove_rows_action is disabled (see update_actions) whenever the
        // selection covers every remaining voice row, so the voice cases
        // below never need to guard against removing the last one
        QUndoCommand* undo_command = nullptr;
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
            undo_command =  // NOLINT(cppcoreguidelines-owning-memory)
                new RemoveVoiceRows<PitchedVoice, PitchedNote>(
                    switch_table.pitched_voices_model, first_row_number,
                    number_of_rows);
            break;
          case RowType::unpitched_voice_type:
            undo_command =  // NOLINT(cppcoreguidelines-owning-memory)
                new RemoveVoiceRows<UnpitchedVoice, UnpitchedNote>(
                    switch_table.unpitched_voices_model, first_row_number,
                    number_of_rows);
            break;
        }
        undo_stack.push(undo_command);
      });
}
