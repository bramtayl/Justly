#pragma once

#include <QtCore/QMimeData>

#include "actions/DeleteCells.hpp"
#include "actions/InsertRemoveRows.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "xml/XMLDocument.hpp"

static void add_delete_cells(SongWidget &song_widget) {
  auto &undo_stack = song_widget.undo_stack;
  auto &switch_column = song_widget.switch_column;

  const auto &range = get_only_range(switch_column);

  QUndoCommand *undo_command = nullptr;
  switch (switch_column.current_row_type) {
  case chord_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.chords_table.chords_model, range);
    break;
  case pitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.pitched_notes_table.pitched_notes_model, range);
    break;
  case unpitched_note_type:
    undo_command = new DeleteCells( // NOLINT(cppcoreguidelines-owning-memory)
        switch_column.unpitched_notes_table.unpitched_notes_model, range);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  undo_stack.push(undo_command);
}

template <RowInterface SubRow>
[[nodiscard]] static auto
make_remove_command(RowsModel<SubRow> &rows_model, const int first_row_number,
                    const int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns(), true);
}

template <RowInterface SubRow>
static void copy_from_model(QMimeData &mime_data,
                            const RowsModel<SubRow> &rows_model,
                            const QItemSelectionRange &range) {
  const auto &rows = rows_model.get_rows();

  const auto first_row_number = range.top();
  const auto left_column = range.left();
  const auto right_column = range.right();

  XMLDocument document;
  auto &root_node = make_root(document, "clipboard");
  set_xml_int(root_node, "left_column", left_column);
  set_xml_int(root_node, "right_column", right_column);
  auto &rows_node = get_new_child(root_node, "rows");
  for (int index = first_row_number;
       index < first_row_number + get_number_of_rows(range); index++) {
    auto &row = rows[index];
    auto &row_node = get_new_child(rows_node, SubRow::get_xml_field_name());
    for (auto column_number = left_column; column_number <= right_column;
         column_number++) {
      row.column_to_xml(row_node, column_number);
    }
  }

  xmlChar *char_buffer = nullptr;
  auto buffer_size = 0;
  xmlDocDumpMemory(document.internal_pointer, &char_buffer, &buffer_size);
  mime_data.setData(
      SubRow::get_cells_mime(),
      reinterpret_cast< // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
          char *>(char_buffer));
}

static void copy_selection(const SwitchColumn &switch_column) {
  const auto &range = get_only_range(switch_column);
  auto &mime_data = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QMimeData);

  switch (switch_column.current_row_type) {
  case chord_type:
    copy_from_model(mime_data, switch_column.chords_table.chords_model, range);
    break;
  case pitched_note_type:
    copy_from_model(mime_data,
                    switch_column.pitched_notes_table.pitched_notes_model,
                    range);
    break;
  case unpitched_note_type:
    copy_from_model(mime_data,
                    switch_column.unpitched_notes_table.unpitched_notes_model,
                    range);
    break;
  default:
    Q_ASSERT(false);
    return;
  }
  get_clipboard().setMimeData(&mime_data);
}

struct EditMenu : public QMenu {
  QAction cut_action = QAction(EditMenu::tr("&Cut"));
  QAction copy_action = QAction(EditMenu::tr("&Copy"));
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action = QAction(EditMenu::tr("&Delete cells"));
  QAction remove_rows_action = QAction(EditMenu::tr("&Remove rows"));

  explicit EditMenu(SongWidget &song_widget)
      : QMenu(EditMenu::tr("&Edit")), paste_menu(PasteMenu(song_widget)),
        insert_menu(InsertMenu(song_widget)) {
    auto &undo_stack = song_widget.undo_stack;
    auto &switch_column = song_widget.switch_column;

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

    QObject::connect(&cut_action, &QAction::triggered, this, [&song_widget]() {
      copy_selection(song_widget.switch_column);
      add_delete_cells(song_widget);
    });

    QObject::connect(&copy_action, &QAction::triggered, &switch_column,
                     [&switch_column]() { copy_selection(switch_column); });

    QObject::connect(&delete_cells_action, &QAction::triggered, this,
                     [&song_widget]() { add_delete_cells(song_widget); });

    QObject::connect(
        &remove_rows_action, &QAction::triggered, this, [&song_widget]() {
          auto &switch_column = song_widget.switch_column;
          auto &undo_stack = song_widget.undo_stack;

          const auto &range = get_only_range(switch_column);
          const auto first_row_number = range.top();
          const auto number_of_rows = get_number_of_rows(range);

          QUndoCommand *undo_command = nullptr;
          switch (switch_column.current_row_type) {
          case chord_type:
            undo_command =
                make_remove_command(switch_column.chords_table.chords_model,
                                    first_row_number, number_of_rows);
            break;
          case pitched_note_type:
            undo_command = make_remove_command(
                switch_column.pitched_notes_table.pitched_notes_model,
                first_row_number, number_of_rows);
            break;
          case unpitched_note_type:
            undo_command = make_remove_command(
                switch_column.unpitched_notes_table.unpitched_notes_model,
                first_row_number, number_of_rows);
            break;
          default:
            Q_ASSERT(false);
            return;
          }
          undo_stack.push(undo_command);
        });
  }
};
