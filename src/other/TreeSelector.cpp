#include "other/TreeSelector.hpp"

#include <QAbstractItemModel>   // for QAbstractItemModel (ptr only), QMod...
#include <QItemSelectionModel>  // for QItemSelection, QItemSelectionModel
#include <QList>                // for QList<>::const_iterator, QList
#include <QtGlobal>

#include "justly/NoteChordField.hpp"

TreeSelector::TreeSelector(QAbstractItemModel* model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection& new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    const auto& first_range = new_selection[0];
    if (first_range.left() == symbol_column) {
      flags = flags | QItemSelectionModel::Rows;
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
    auto selected_indices = new_selection.indexes();
    Q_ASSERT(!selected_indices.empty());
    auto first_selected_index = selected_indices[0];
    return QItemSelectionModel::select(
        QItemSelection(first_selected_index, first_selected_index),
        QItemSelectionModel::Select | QItemSelectionModel::Clear);
  }

  return QItemSelectionModel::select(new_selection, flags);
}

void TreeSelector::select(const QModelIndex& new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};