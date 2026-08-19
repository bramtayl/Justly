#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QKeySequence>
#include <QtGui/QUndoStack>
#include <QtWidgets/QMenu>

#include "actions/DeleteCells.hpp"
#include "actions/InsertRemoveRows.hpp"
#include "actions/RemoveVoiceRows.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "models/ChordsModel.hpp"
#include "models/PitchedNotesModel.hpp"
#include "models/PitchedVoicesModel.hpp"
#include "models/RowsModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "models/UnpitchedVoicesModel.hpp"
#include "other/helpers.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/PitchedVoice.hpp"
#include "rows/Row.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "rows/UnpitchedVoice.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"
#include "xml/XMLDocument.hpp"

template <RowInterface SubRow>
[[nodiscard]] static auto
make_remove_command(RowsModel<SubRow> &rows_model, const int first_row_number,
                    const int number_of_rows) -> QUndoCommand * {
  return new InsertRemoveRows( // NOLINT(cppcoreguidelines-owning-memory)
      rows_model, first_row_number,
      copy_items(rows_model.get_rows(), first_row_number, number_of_rows), 0,
      SubRow::get_number_of_columns() - 1, true);
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

  mime_data.setData(SubRow::get_cells_mime(), document_to_byte_array(document));
}

struct EditMenu : public QMenu {
  QAction cut_action = QAction(EditMenu::tr("&Cut"));
  QAction copy_action = QAction(EditMenu::tr("&Copy"));
  PasteMenu paste_menu;
  InsertMenu insert_menu;
  QAction delete_cells_action = QAction(EditMenu::tr("&Delete cells"));
  QAction remove_rows_action = QAction(EditMenu::tr("&Remove rows"));

  explicit EditMenu(SongWidget &song_widget);
};
