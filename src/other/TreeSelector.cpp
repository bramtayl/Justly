#include "other/TreeSelector.hpp"

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QList>
#include <QtGlobal>
#include <algorithm>

#include "justly/NoteChordColumn.hpp"
#include "other/ChordsModel.hpp"
#include "other/conversions.hpp"

auto is_rows(const QItemSelection &selection) -> bool {
  // selecting a type column selects the whole row
  Q_ASSERT(!selection.isEmpty());
  return selection[0].left() == type_column;
}

auto get_first_child_number(const QItemSelectionRange &range) -> size_t {
  return to_size_t(range.top());
}

auto get_number_of_children(const QItemSelectionRange &range) -> size_t {
  return to_size_t(range.bottom() - range.top() + 1);
}

TreeSelector::TreeSelector(QAbstractItemModel *model)
    : QItemSelectionModel(model) {}

void TreeSelector::select(const QItemSelection &new_selection,
                          QItemSelectionModel::SelectionFlags command) {
  auto flags = QFlags(command);

  if (!new_selection.empty() && flags.testFlag(QItemSelectionModel::Select)) {
    auto already_selected = selection();
    QItemSelection revised_selection;
    const auto &reference_selection =
        (already_selected.empty() || flags.testFlag(QItemSelectionModel::Clear))
            ? new_selection
            : already_selected;
    auto min_pointer = std::min_element(
        reference_selection.begin(), reference_selection.end(),
        [](const QItemSelectionRange &range_1,
           const QItemSelectionRange &range_2) {
          auto parent_index_1 = range_1.parent();
          auto range_1_is_chords = is_root_index(parent_index_1);
          auto parent_index_2 = range_2.parent();
          auto range_2_is_chords = is_root_index(parent_index_2);
          if (range_1_is_chords) {
            auto range_1_chord_number = get_first_child_number(range_1);
            if (range_2_is_chords) {
              return range_1_chord_number < get_first_child_number(range_2);
            }
            // chord is less than note 2 if they are tied
            return range_1_chord_number <= get_child_number(parent_index_2);
          }
          auto range_1_chord_number = get_child_number(parent_index_1);
          if (range_2_is_chords) {
            // note isn't less than chord 2 if they are tied
            return range_1_chord_number < get_first_child_number(range_2);
          }
          return range_1_chord_number < get_child_number(parent_index_2);
        });
    Q_ASSERT(min_pointer != nullptr);
    const auto &parent_index = min_pointer->parent();
    for (const auto &range : new_selection) {
      if (range.parent() == parent_index) {
        revised_selection.append(range);
      }
    }
    return QItemSelectionModel::select(revised_selection,
                                       is_rows(reference_selection)
                                           ? flags | QItemSelectionModel::Rows
                                           : flags);
  }
  return QItemSelectionModel::select(new_selection, flags);
}

void TreeSelector::select(const QModelIndex &new_index,
                          QItemSelectionModel::SelectionFlags command) {
  return select(QItemSelection(new_index, new_index), command);
};