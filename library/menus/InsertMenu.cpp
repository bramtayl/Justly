#include "menus/InsertMenu.hpp"

#include "menus/MenuAction.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/WindowBody.hpp"

void add_insert_row(WindowBody& window_body, const int row_number) {
  window_body.undo_stack.push(
      dispatch_row_type(window_body.switch_column.switch_table,
                        [row_number](auto& rows_model) -> QUndoCommand* {
                          return make_insert_command(rows_model, row_number);
                        }));
}

InsertMenu::InsertMenu(WindowBody& window_body)
    : QMenu(InsertMenu::tr("&Insert row")),
      insert_after_action(InsertMenu::tr("&After")),
      insert_into_start_action(InsertMenu::tr("&Into start")) {
  add_menu_action(*this, insert_after_action, QKeySequence::InsertLineSeparator,
                  false);
  add_menu_action(*this, insert_into_start_action, QKeySequence::AddTab);

  QObject::connect(&insert_after_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     add_insert_row(window_body, get_next_row(window_body));
                   });

  QObject::connect(
      &insert_into_start_action, &QAction::triggered, this,
      [&window_body]() -> auto { add_insert_row(window_body, 0); });
}
