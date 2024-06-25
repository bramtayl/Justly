#include "TreeSelector.hpp"

#include <qitemselectionmodel.h>

TreeSelector::TreeSelector(QAbstractItemModel* model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection& new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    auto already_selected = selection();
    QItemSelection revised_selection;
    auto parent_index =
        flags.testFlag(QItemSelectionModel::Clear) || already_selected.empty()
            ? new_selection[0].parent()
            : already_selected[0].parent();
    for (const auto& range : new_selection) {
      if (range.parent() == parent_index) {
        revised_selection.append(range);
      }
    }
    return QItemSelectionModel::select(revised_selection, command);
  }

  return QItemSelectionModel::select(new_selection, command);
}

void TreeSelector::select(const QModelIndex& new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};