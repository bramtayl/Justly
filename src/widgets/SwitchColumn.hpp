#pragma once

#include "rows/RowType.hpp"
#include "widgets/SwitchTable.hpp"

struct SwitchColumn : public QWidget {
  QLabel &editing_text = *(new QLabel(SwitchColumn::tr("Chords")));
  SwitchTable &switch_table;

  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  SwitchColumn(QUndoStack &undo_stack, Song &song)
      : switch_table(*new SwitchTable(undo_stack, song)) {
    column_layout.addWidget(&editing_text);
    column_layout.addWidget(&switch_table);
  }
};

[[nodiscard]] static inline auto
get_parent_chord_number(const SwitchTable &switch_table) -> int {
  switch (switch_table.current_row_type) {
  case chord_type:
    return -1;
  case pitched_note_type:
    return switch_table.pitched_notes_model.parent_chord_number;
  case unpitched_note_type:
    return switch_table.unpitched_notes_model.parent_chord_number;
  default:
    Q_ASSERT(false);
    return -1;
  }
}

template <typename Iterable>
[[nodiscard]] static auto get_only(const Iterable &iterable) -> const auto & {
  Q_ASSERT(iterable.size() == 1);
  return iterable.at(0);
}

[[nodiscard]] static inline auto get_selection_model(
    const QAbstractItemView &item_view) -> QItemSelectionModel & {
  return get_reference(item_view.selectionModel());
}

[[nodiscard]] static inline auto
get_only_range(const QAbstractItemView &table) {
  return get_only(get_selection_model(table).selection());
}
