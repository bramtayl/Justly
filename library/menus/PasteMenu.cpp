#include "menus/PasteMenu.hpp"

#include "menus/MenuAction.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

auto get_mime_description(const QString& mime_type) -> QString {
  Q_ASSERT(mime_type.isValidUtf16());
  if (mime_type == Chord::get_cells_mime()) {
    return QObject::tr("chords cells");
  }
  if (mime_type == PitchedNote::get_cells_mime()) {
    return QObject::tr("pitched notes cells");
  }
  if (mime_type == UnpitchedNote::get_cells_mime()) {
    return QObject::tr("unpitched notes cells");
  }
  if (mime_type == PitchedVoice::get_cells_mime()) {
    return QObject::tr("pitched voices cells");
  }
  if (mime_type == UnpitchedVoice::get_cells_mime()) {
    return QObject::tr("unpitched voices cells");
  }
  return mime_type;
}

namespace {

void add_paste_insert(WindowBody& window_body, const int row_number) {
  auto& switch_column = window_body.switch_column;
  auto& switch_table = switch_column.switch_table;

  auto* undo_command = dispatch_row_type(
      switch_table,
      [&switch_table, row_number](auto& rows_model) -> QUndoCommand* {
        return make_paste_insert_command(switch_table, rows_model, row_number);
      });
  if (undo_command == nullptr) {
    return;
  }
  window_body.undo_stack.push(undo_command);
}

}  // namespace

PasteMenu::PasteMenu(WindowBody& window_body)
    : QMenu(PasteMenu::tr("&Paste")),
      paste_over_action(PasteMenu::tr("&Over")),
      paste_into_start_action(PasteMenu::tr("&Into start")),
      paste_after_action(PasteMenu::tr("&After")) {
  add_menu_action(*this, paste_over_action, QKeySequence::Paste, false);
  add_menu_action(*this, paste_into_start_action);
  add_menu_action(*this, paste_after_action, QKeySequence::UnknownKey, false);
  paste_after_action.setShortcut(Qt::ControlModifier | Qt::ShiftModifier |
                                 Qt::Key_V);

  QObject::connect(
      &paste_over_action, &QAction::triggered, this, [&window_body]() -> auto {
        auto& switch_table = window_body.switch_column.switch_table;

        const auto first_row_number = get_only_range(switch_table).top();

        auto* undo_command =
            dispatch_row_type(switch_table,
                              [&switch_table, first_row_number](
                                  auto& rows_model) -> QUndoCommand* {
                                return make_paste_cells_command(
                                    switch_table, first_row_number, rows_model);
                              });
        if (undo_command == nullptr) {
          return;
        }
        window_body.undo_stack.push(undo_command);
      });

  QObject::connect(
      &paste_into_start_action, &QAction::triggered, this,
      [&window_body]() -> auto { add_paste_insert(window_body, 0); });

  QObject::connect(&paste_after_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     add_paste_insert(window_body, get_next_row(window_body));
                   });
}
