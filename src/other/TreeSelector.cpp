#include "other/TreeSelector.hpp"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QList>
#include <QtGlobal>

#include "justly/NoteChordField.hpp"
#include "justly/RowRange.hpp"

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
      auto parent_number = 0;
      if (already_selected.empty() ||
          flags.testFlag(QItemSelectionModel::Clear)) {
        auto new_row_ranges = to_row_ranges(new_selection);
        Q_ASSERT(!new_row_ranges.empty());
        parent_number = new_row_ranges[0].parent_number;
      } else {
        auto old_row_ranges = to_row_ranges(already_selected);
        Q_ASSERT(!old_row_ranges.empty());
        parent_number = old_row_ranges[0].parent_number;
      }
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