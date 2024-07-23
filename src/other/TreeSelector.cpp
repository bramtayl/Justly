#include "other/TreeSelector.hpp"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QList>
#include <QtGlobal>

#include "justly/NoteChordField.hpp"

TreeSelector::TreeSelector(QAbstractItemModel *model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection &new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  qInfo("here!");
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    qInfo("here 2!");
    const auto &first_range = new_selection[0];
    if (first_range.left() == type_column) {
      auto new_flags = flags | QItemSelectionModel::Rows;
      auto already_selected = selection();
      QItemSelection revised_selection;
      auto parent_index = (already_selected.empty() ||
                           flags.testFlag(QItemSelectionModel::Clear))
                              ? first_range.parent()
                              : already_selected[0].parent();
      for (const auto &range : new_selection) {
        if (range.parent() == parent_index) {
          revised_selection.append(range);
        }
      }
      return QItemSelectionModel::select(revised_selection, new_flags);
    }
  }
  return QItemSelectionModel::select(new_selection, flags);
}

void TreeSelector::select(const QModelIndex &new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};