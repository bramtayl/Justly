#include "widgets/SwitchColumn.hpp"

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

#include "widgets/SwitchTable.hpp"

SwitchColumn::SwitchColumn(QUndoStack& undo_stack, Song& song)
    : editing_text(*(new QLabel(SwitchColumn::tr("Chords")))),
      switch_table(*new SwitchTable(undo_stack, song)) {
  auto& column_layout =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QVBoxLayout(this));
  column_layout.addWidget(&editing_text);
  column_layout.addWidget(&switch_table);
}

auto get_parent_chord_number(const SwitchTable& switch_table) -> int {
  // -1 for the chords and voices models, which never get a parent chord
  return dispatch_row_type(switch_table, [](const auto& rows_model) -> int {
    return rows_model.parent_chord_number;
  });
}

auto get_selection_model(const QAbstractItemView& item_view)
    -> QItemSelectionModel& {
  return get_reference(item_view.selectionModel());
}

auto get_only_range(const QAbstractItemView& table) -> QItemSelectionRange {
  return get_only(get_selection_model(table).selection());
}

void select_rows_and_scroll(QAbstractItemView& item_view,
                            const QModelIndex& first_index,
                            const QModelIndex& last_index,
                            const QModelIndex& scroll_index) {
  get_selection_model(item_view).select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Clear |
          QItemSelectionModel::Rows);
  item_view.scrollTo(scroll_index);
}

void select_row_and_scroll(QAbstractItemView& item_view,
                           const QModelIndex& row_index) {
  select_rows_and_scroll(item_view, row_index, row_index, row_index);
}
