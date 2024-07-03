#include "other/TreeSelector.hpp"

#include <qabstractitemmodel.h>   // for QAbstractItemModel (ptr only), QMod...
#include <qflags.h>               // for QFlags
#include <qitemselectionmodel.h>  // for QItemSelection, QItemSelectionModel
#include <qlist.h>                // for QList<>::const_iterator, QList

#include "justly/NoteChordField.hpp"

TreeSelector::TreeSelector(QAbstractItemModel* model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection& new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    // TODO: only one cell at a time
    const auto &first_range = new_selection[0];
    if (first_range.left() == symbol_column) {
      flags = flags | QItemSelectionModel::Rows;
    }
    auto already_selected = selection();
    QItemSelection revised_selection;
    auto parent_index =
        flags.testFlag(QItemSelectionModel::Clear) || already_selected.empty()
            ? first_range.parent()
            : already_selected[0].parent();
    for (const auto& range : new_selection) {
      if (range.parent() == parent_index) {
        revised_selection.append(range);
      }
    }
    return QItemSelectionModel::select(revised_selection, flags);
  }

  return QItemSelectionModel::select(new_selection, flags);
}

void TreeSelector::select(const QModelIndex& new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};