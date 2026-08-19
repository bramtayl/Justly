#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtCore/QMimeData>
#include <QtGui/QAction>
#include <QtWidgets/QMenu>

#include "actions/InsertRemoveRows.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "models/RowsModel.hpp"
#include "other/helpers.hpp"
#include "rows/Row.hpp"
#include "xml/XMLDocument.hpp"

class QUndoCommand;
struct SongWidget;

template <RowInterface SubRow>
[[nodiscard]] static auto make_remove_command(RowsModel<SubRow>& rows_model,
                                              const int first_row_number,
                                              const int number_of_rows)
    -> QUndoCommand* {
  return new InsertRemoveRows(  // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns() - 1, true);
}

template <RowInterface SubRow>
static void copy_from_model(QMimeData& mime_data,
                            const RowsModel<SubRow>& rows_model,
                            const QItemSelectionRange& range) {
  const auto& rows = rows_model.get_rows();

  const auto first_row_number = range.top();
  const auto left_column = range.left();
  const auto right_column = range.right();

  XMLDocument document;
  auto& root_node = make_root(document, "clipboard");
  set_xml_int(root_node, "left_column", left_column);
  set_xml_int(root_node, "right_column", right_column);
  auto& rows_node = get_new_child(root_node, "rows");
  for (int index = first_row_number;
       index < first_row_number + get_number_of_rows(range); index++) {
    auto& row = rows[index];
    auto& row_node = get_new_child(rows_node, SubRow::get_xml_field_name());
    for (auto column_number = left_column; column_number <= right_column;
         column_number++) {
      row.column_to_xml(row_node, column_number);
    }
  }

  mime_data.setData(SubRow::get_cells_mime(), document_to_byte_array(document));
}

struct EditMenu : public QMenu {
  QAction cut_action;
  QAction copy_action;
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action;
  QAction remove_rows_action;

  explicit EditMenu(SongWidget& song_widget);
};
