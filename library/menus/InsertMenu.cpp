#include "menus/InsertMenu.hpp"

#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/WindowBody.hpp"

void add_insert_row(WindowBody& window_body, const int row_number,
                    const RowType new_row_type) {
  auto& switch_table = window_body.switch_column.switch_table;
  QUndoCommand* undo_command = nullptr;
  const auto& chords = window_body.song.chords;
  switch (new_row_type) {
    case RowType::chord_type:
      undo_command = make_insert_row(switch_table.chords_model, row_number);
      break;
    case RowType::pitched_note_type:
      undo_command = make_insert_note(switch_table.pitched_notes_model, chords,
                                      row_number);
      break;
    case RowType::unpitched_note_type:
      undo_command = make_insert_note(switch_table.unpitched_notes_model,
                                      chords, row_number);
      break;
    case RowType::pitched_voice_type:
      undo_command =
          make_insert_voice(switch_table.pitched_voices_model, row_number);
      break;
    case RowType::unpitched_voice_type:
      undo_command =
          make_insert_voice(switch_table.unpitched_voices_model, row_number);
      break;
  }
  window_body.undo_stack.push(undo_command);
}

namespace {

void add_insert_row_default(WindowBody& window_body, const int row_number) {
  add_insert_row(
      window_body, row_number,
      window_body.switch_column.switch_table.delegate.current_row_type);
}

}  // namespace

InsertMenu::InsertMenu(WindowBody& window_body)
    : QMenu(InsertMenu::tr("&Insert row")),
      insert_after_action(InsertMenu::tr("&After")),
      insert_into_start_action(InsertMenu::tr("&Into start")) {
  add_menu_action(*this, insert_after_action, QKeySequence::InsertLineSeparator,
                  false);
  add_menu_action(*this, insert_into_start_action, QKeySequence::AddTab);

  QObject::connect(&insert_after_action, &QAction::triggered, this,
                   [&window_body]() -> auto {
                     add_insert_row_default(window_body,
                                            get_next_row(window_body));
                   });

  QObject::connect(
      &insert_into_start_action, &QAction::triggered, this,
      [&window_body]() -> auto { add_insert_row_default(window_body, 0); });
}
