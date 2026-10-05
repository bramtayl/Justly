#include "menus/EditMenu.hpp"

#include "actions/SetCells.hpp"
#include "menus/MenuAction.hpp"
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
            range.right(),
            QList<decltype(empty_row)>(number_of_rows, empty_row));
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
        const auto& range = get_only_range(switch_table);

        // remove_rows_action is disabled (see update_actions) whenever the
        // selection covers every remaining voice row, so removing voices
        // never needs to guard against removing the last one
        window_body.undo_stack.push(dispatch_row_type(
            switch_table, [&range](auto& rows_model) -> QUndoCommand* {
              return make_remove_command(rows_model, range.top(),
                                         get_number_of_rows(range));
            }));
      });
}
