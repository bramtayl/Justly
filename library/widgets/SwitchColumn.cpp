#include "widgets/SwitchColumn.hpp"

#include <QtWidgets/QAbstractItemView>

#include "models/PitchedNotesModel.hpp"
#include "models/UnpitchedNotesModel.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"

SwitchColumn::SwitchColumn(QUndoStack &undo_stack, Song &song)
    : switch_table(*new SwitchTable(undo_stack, song)) {
  column_layout.addWidget(&editing_text);
  column_layout.addWidget(&switch_table);
}

auto get_parent_chord_number(const SwitchTable &switch_table) -> int {
  switch (switch_table.delegate.current_row_type) {
  case RowType::chord_type:
  case RowType::pitched_voice_type:
  case RowType::unpitched_voice_type:
    return -1;
  case RowType::pitched_note_type:
    return switch_table.pitched_notes_model.parent_chord_number;
  case RowType::unpitched_note_type:
    return switch_table.unpitched_notes_model.parent_chord_number;
  }
  Q_UNREACHABLE();
}

auto get_selection_model(const QAbstractItemView &item_view) -> QItemSelectionModel & {
  return get_reference(item_view.selectionModel());
}

auto get_only_range(const QAbstractItemView &table) -> QItemSelectionRange {
  return get_only(get_selection_model(table).selection());
}
