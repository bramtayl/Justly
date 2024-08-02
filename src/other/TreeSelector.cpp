#include "other/TreeSelector.hpp"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QList>
#include <QtGlobal>

#include "indices/index_functions.hpp"
#include "justly/NoteChordColumn.hpp"
#include "indices/RowRange.hpp"

TreeSelector::TreeSelector(QAbstractItemModel *model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection &new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    const auto &first_range = new_selection[0];
    if (first_range.left() == type_column) {
      auto already_selected = selection();
      QItemSelection revised_selection;
      auto parent_number =
          get_first_row_range((already_selected.empty() ||
                               flags.testFlag(QItemSelectionModel::Clear))
                                  ? new_selection
                                  : already_selected)
              .parent_number;
      for (const auto &range : new_selection) {
        if (range.parent().row() == parent_number) {
          revised_selection.append(range);
        }
      }
      return QItemSelectionModel::select(revised_selection,
                                         flags | QItemSelectionModel::Rows);
    }
  }
  return QItemSelectionModel::select(new_selection, flags);
}

void TreeSelector::select(const QModelIndex &new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};