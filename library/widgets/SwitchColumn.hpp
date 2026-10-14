#pragma once

#include <QtCore/QItemSelectionModel>
#include <QtWidgets/QWidget>

#include "models/TableSelection.hpp"

class QLabel;
class QUndoStack;
struct Song;
class QAbstractItemView;
struct SwitchTable;

struct SwitchColumn : public QWidget {
  QLabel& editing_text;
  SwitchTable& switch_table;

  SwitchColumn(QUndoStack& undo_stack, Song& song);
};

[[nodiscard]] auto get_parent_chord_number(const SwitchTable& switch_table)
    -> int;

template <typename Iterable>
[[nodiscard]] auto get_only(const Iterable& iterable) -> const auto& {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

[[nodiscard]] auto get_selection_model(const QAbstractItemView& item_view)
    -> QItemSelectionModel&;

[[nodiscard]] auto get_only_range(const QAbstractItemView& table)
    -> QItemSelectionRange;

// the switch table's current selection; empty (number_of_rows == 0) if
// nothing is selected
[[nodiscard]] auto get_table_selection(const SwitchTable& switch_table)
    -> TableSelection;

// replaces the selection with the whole rows from first_index's through
// last_index's, then scrolls to scroll_index
void select_rows_and_scroll(QAbstractItemView& item_view,
                            const QModelIndex& first_index,
                            const QModelIndex& last_index,
                            const QModelIndex& scroll_index);

void select_row_and_scroll(QAbstractItemView& item_view,
                           const QModelIndex& row_index);
