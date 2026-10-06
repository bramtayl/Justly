#pragma once

#include "actions/RemoveVoiceRows.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"

template <RowInterface SubRow>
[[nodiscard]] auto make_remove_command(RowsModel<SubRow>& rows_model,
                                       const int first_row_number,
                                       const int number_of_rows)
    -> QUndoCommand* {
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns() - 1, true);
}

// removing voices also reassigns their notes
template <VoiceInterface SubVoice, NoteInterface SubNote>
[[nodiscard]] auto make_remove_command(
    VoicesModel<SubVoice, SubNote>& voices_model, const int first_row_number,
    const int number_of_rows) -> QUndoCommand* {
  return new RemoveVoiceRows<  // NOLINT(cppcoreguidelines-owning-memory)
      SubVoice, SubNote>(voices_model, first_row_number, number_of_rows);
}

template <RowInterface SubRow>
void copy_from_model(QMimeData& mime_data, const RowsModel<SubRow>& rows_model,
                     const QItemSelectionRange& range) {
  const auto left_column = range.left();
  const auto right_column = range.right();

  XMLDocument document;
  auto& root_node = make_root(document, "clipboard");
  set_xml_int(root_node, "left_column", left_column);
  set_xml_int(root_node, "right_column", right_column);
  rows_to_xml(
      get_new_child(root_node, "rows"),
      copy_items(rows_model.get_rows(), range.top(), get_number_of_rows(range)),
      left_column, right_column);

  mime_data.setData(SubRow::get_cells_mime(), document_to_byte_array(document));
}

struct EditMenu : public QMenu {
  QAction cut_action;
  QAction copy_action;
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action;
  QAction remove_rows_action;

  explicit EditMenu(WindowBody& window_body);
};
