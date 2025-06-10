#pragma once

#include "ChordsTable.hpp"
#include "PitchedNotesTable.hpp"
#include "RowType.hpp"
#include "UnpitchedNotesTable.hpp"

struct SwitchColumn : public QWidget {
  RowType current_row_type = chord_type;

  QLabel &editing_text = *(new QLabel(SwitchColumn::tr("Chords")));
  ChordsTable &chords_table;
  PitchedNotesTable &pitched_notes_table;
  UnpitchedNotesTable &unpitched_notes_table;

  QBoxLayout &column_layout = *(new QVBoxLayout(this));

  SwitchColumn(QUndoStack &undo_stack, Song &song)
      : chords_table(*new ChordsTable(undo_stack, song)),
        pitched_notes_table(*new PitchedNotesTable(undo_stack, song)),
        unpitched_notes_table(*new UnpitchedNotesTable(undo_stack)) {
    column_layout.addWidget(&editing_text);
    column_layout.addWidget(&chords_table);
    column_layout.addWidget(&pitched_notes_table);
    column_layout.addWidget(&unpitched_notes_table);
  }
};

[[nodiscard]] static auto
get_table(const SwitchColumn &switch_column) -> QTableView & {
  auto &chords_table = switch_column.chords_table;
  switch (switch_column.current_row_type) {
  case chord_type:
    return chords_table;
  case pitched_note_type:
    return switch_column.pitched_notes_table;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table;
  default:
    Q_ASSERT(false);
    return chords_table;
  }
}

[[nodiscard]] static inline auto 
get_parent_chord_number(const SwitchColumn &switch_column) -> int {
  switch (switch_column.current_row_type) {
  case chord_type:
    return -1;
  case pitched_note_type:
    return switch_column.pitched_notes_table.model.parent_chord_number;
  case unpitched_note_type:
    return switch_column.unpitched_notes_table.model.parent_chord_number;
  default:
    Q_ASSERT(false);
    return -1;
  }
}

[[nodiscard]] static inline auto get_only_range(const SwitchColumn &switch_column) {
  return get_only(get_selection_model(get_table(switch_column)).selection());
}
