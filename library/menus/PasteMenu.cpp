#include "menus/PasteMenu.hpp"

#include "menus/MenuAction.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

auto get_mime_description(const QString& mime_type) -> QString {
  Q_ASSERT(mime_type.isValidUtf16());
  static const QMap<QString, QString> descriptions{
      {Chord::get_cells_mime(), QObject::tr("chords cells")},
      {PitchedNote::get_cells_mime(), QObject::tr("pitched notes cells")},
      {UnpitchedNote::get_cells_mime(), QObject::tr("unpitched notes cells")},
      {PitchedVoice::get_cells_mime(), QObject::tr("pitched voices cells")},
      {UnpitchedVoice::get_cells_mime(), QObject::tr("unpitched voices cells")},
  };
  // a type from another program is shown as is
  return descriptions.value(mime_type, mime_type);
}

namespace {

void add_paste_insert(WindowBody& window_body, const int row_number) {
  auto& switch_column = window_body.switch_column;
  auto& switch_table = switch_column.switch_table;

  maybe_push(window_body.undo_stack,
             dispatch_row_type(switch_table,
                               [&switch_table,
                                row_number](auto& rows_model) -> QUndoCommand* {
                                 return make_paste_insert_command(
                                     switch_table, rows_model, row_number);
                               }));
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

        maybe_push(window_body.undo_stack,
                   dispatch_row_type(switch_table,
                                     [&switch_table, first_row_number](
                                         auto& rows_model) -> QUndoCommand* {
                                       return make_paste_cells_command(
                                           switch_table, first_row_number,
                                           rows_model);
                                     }));
      });

  QObject::connect(
      &paste_into_start_action, &QAction::triggered, this,
      [&window_body]() -> auto { add_paste_insert(window_body, 0); });

  QObject::connect(&paste_after_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     add_paste_insert(window_body, get_next_row(window_body));
                   });
}
