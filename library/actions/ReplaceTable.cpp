#include "actions/ReplaceTable.hpp"

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <memory>
#include <set>

#include "actions/ChangeId.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/SongMenuBar.hpp"
#include "piano_roll/PianoRollWidget.hpp"
#include "widgets/ControlsColumn.hpp"

namespace {

// fits each column to its contents, but at least as wide as its editor (or,
// for the text columns, TEXT_WIDTH), and makes rows as tall as the tallest
// editor, so editing a cell doesn't squash it
void resize_cells(SwitchTable& switch_table, const RowType row_type) {
  static const auto TEXT_WIDTH = 200;
  static const std::set<std::pair<RowType, int>> TEXT_COLUMNS = {
      {RowType::chord_type, static_cast<int>(ChordColumn::chord_words_column)},
      {RowType::pitched_note_type,
       static_cast<int>(PitchedNoteColumn::pitched_note_words_column)},
      {RowType::unpitched_note_type,
       static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column)},
      {RowType::pitched_voice_type,
       static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)},
      {RowType::unpitched_voice_type,
       static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)},
  };
  // the table takes the grid line out of the cell the editor fills
  const auto grid_width = switch_table.showGrid() ? 1 : 0;
  auto row_height = 0;
  const auto number_of_columns =
      get_reference(switch_table.model()).columnCount();
  for (auto column = 0; column < number_of_columns; column++) {
    switch_table.resizeColumnToContents(column);
    auto minimum_width = 0;
    const std::unique_ptr<QWidget> editor_pointer(
        create_switch_editor(switch_table.delegate, nullptr, row_type, column));
    if (editor_pointer != nullptr) {
      const auto editor_size = editor_pointer->sizeHint();
      minimum_width = editor_size.width() + grid_width;
      row_height = std::max(row_height, editor_size.height() + grid_width);
    }
    if (TEXT_COLUMNS.contains({row_type, column})) {
      minimum_width = std::max(minimum_width, TEXT_WIDTH);
    }
    switch_table.setColumnWidth(
        column, std::max(minimum_width, switch_table.columnWidth(column)));
  }
  get_reference(switch_table.verticalHeader())
      .setDefaultSectionSize(row_height);
}

template <RowInterface SubRow>
void show_model(SwitchTable& switch_table, RowsModel<SubRow>& rows_model,
                const RowType row_type) {
  set_model(switch_table, rows_model);
  resize_cells(switch_table, row_type);
}

// points notes_model at one chord's notes, swapping it into the table if it
// isn't already there, and selects new_note_number's row if it's >= 0
template <NoteInterface SubNote>
void show_notes(SwitchTable& switch_table, RowsModel<SubNote>& notes_model,
                QList<SubNote>& notes, const RowType row_type,
                const bool row_type_changed, const int chord_number,
                const int new_note_number) {
  notes_model.set_rows_pointer(&notes, chord_number);
  if (row_type_changed) {
    show_model(switch_table, notes_model, row_type);
  }
  if (new_note_number >= 0) {
    select_row_and_scroll(switch_table, notes_model.index(new_note_number, 0));
  }
}

// voice names must be typed, not copy/pasted or deleted, since every voice
// name must stay unique and non-empty
template <RowInterface SubRow>
auto selects_voice_name(const RowsModel<SubRow>& /*rows_model*/,
                        const QItemSelection& selection) -> bool {
  if constexpr (VoiceInterface<SubRow>) {
    return std::ranges::any_of(
        selection, [](const QItemSelectionRange& range) -> bool {
          return range.contains(range.top(), SubRow::get_name_column(),
                                range.parent());
        });
  } else {
    return false;
  }
}

// removing every remaining voice row would leave no voice for a note to
// reference, so disable rather than let RemoveVoiceRows warn and cancel;
// selection.size() can transiently be 0 or >1 while a model change (e.g.
// undoing a row removal) is still adjusting the selection, so require
// exactly one range rather than asserting via get_only_range
template <RowInterface SubRow>
auto removes_every_voice_row(const RowsModel<SubRow>& rows_model,
                             const QItemSelection& selection) -> bool {
  if constexpr (VoiceInterface<SubRow>) {
    return selection.size() == 1 &&
           get_number_of_rows(selection.at(0)) >= rows_model.get_rows().size();
  } else {
    return false;
  }
}

// mirrors the switch table's current selection onto the piano roll (which
// note bar(s) get highlighted, where the cursor jumps to); an empty
// selection clears both, since get_only_range() asserts on an empty range
void update_piano_roll_selection(PianoRollWidget& piano_roll_widget,
                                 const WindowBody& window_body) {
  const auto& switch_table = window_body.switch_column.switch_table;
  TableSelection selection{
      .row_type = switch_table.delegate.current_row_type,
      .chord_number = get_parent_chord_number(switch_table)};
  if (!get_selection_model(switch_table).selection().empty()) {
    const auto& range = get_only_range(switch_table);
    selection.first_row_number = range.top();
    selection.number_of_rows = get_number_of_rows(range);
  }
  set_piano_roll_selection(piano_roll_widget, selection);
}

void update_actions(SongMenuBar& song_menu_bar, WindowBody& window_body,
                    const QItemSelectionModel& selector) {
  auto& edit_menu = song_menu_bar.edit_menu;
  auto& controls_column = window_body.controls_column;

  const auto selection = selector.selection();

  const auto anything_selected = !selection.empty();

  const auto& switch_table = window_body.switch_column.switch_table;

  const auto current_row_type = switch_table.delegate.current_row_type;
  const auto is_voice = is_voice_type(current_row_type);

  // only chords and pitched notes have intervals
  set_interval_rows_are_enabled(
      controls_column,
      anything_selected && (current_row_type == RowType::chord_type ||
                            current_row_type == RowType::pitched_note_type));

  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);
  song_menu_bar.play_menu.play_to_end_action.setEnabled(anything_selected &&
                                                        !is_voice);

  const auto name_column_selected = dispatch_row_type(
      switch_table, [&selection](const auto& rows_model) -> bool {
        return selects_voice_name(rows_model, selection);
      });
  const auto can_copy_paste = anything_selected && !name_column_selected;

  edit_menu.cut_action.setEnabled(can_copy_paste);
  edit_menu.copy_action.setEnabled(can_copy_paste);
  auto& paste_menu = edit_menu.paste_menu;
  paste_menu.paste_over_action.setEnabled(can_copy_paste);
  // pasting after/into always inserts a brand new row built only from the
  // pasted column(s), so for voices it would create one with an empty
  // (invalid) name -- unlike paste_over, which only ever touches existing,
  // already-named rows, and rejects pasted names
  paste_menu.paste_after_action.setEnabled(can_copy_paste && !is_voice);
  paste_menu.paste_into_start_action.setEnabled(!is_voice);
  edit_menu.delete_cells_action.setEnabled(can_copy_paste);
  const auto removing_every_voice_row = dispatch_row_type(
      switch_table, [&selection](const auto& rows_model) -> bool {
        return removes_every_voice_row(rows_model, selection);
      });
  edit_menu.remove_rows_action.setEnabled(anything_selected &&
                                          !removing_every_voice_row);

  edit_menu.insert_menu.insert_after_action.setEnabled(anything_selected);
}

}  // namespace

void replace_table(SongMenuBar& song_menu_bar, WindowBody& window_body,
                   const RowType new_row_type, const int new_chord_number,
                   PianoRollWidget& piano_roll_widget,
                   const int new_note_number) {
  auto& switch_column = window_body.switch_column;
  auto& switch_table = switch_column.switch_table;
  auto& view_menu = song_menu_bar.view_menu;

  auto& chords = window_body.song.chords;
  const auto to_notes = is_note_type(new_row_type);

  const auto old_row_type = switch_table.delegate.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  view_menu.previous_chord_action.setEnabled(to_notes && new_chord_number > 0);
  view_menu.next_chord_action.setEnabled(to_notes &&
                                         new_chord_number < chords.size() - 1);
  view_menu.back_to_chords_action.setEnabled(new_row_type !=
                                             RowType::chord_type);
  view_menu.edit_pitched_voices_action.setEnabled(new_row_type !=
                                                  RowType::pitched_voice_type);
  view_menu.edit_unpitched_voices_action.setEnabled(
      new_row_type != RowType::unpitched_voice_type);

  QString label_text;
  QTextStream stream(&label_text);

  if (new_row_type == RowType::chord_type) {
    stream << SongMenuBar::tr("Chords");

    const auto old_parent_chord_number = get_parent_chord_number(switch_table);

    show_model(switch_table, switch_table.chords_model, new_row_type);

    if (old_parent_chord_number >= 0) {
      select_row_and_scroll(switch_table, switch_table.chords_model.index(
                                              old_parent_chord_number, 0));
    }

    switch (old_row_type) {
      case RowType::chord_type:
      case RowType::pitched_voice_type:
      case RowType::unpitched_voice_type:
        break;
      case RowType::pitched_note_type:
        switch_table.pitched_notes_model.set_rows_pointer();
        break;
      case RowType::unpitched_note_type:
        switch_table.unpitched_notes_model.set_rows_pointer();
        break;
    }
  } else if (new_row_type == RowType::pitched_voice_type) {
    stream << SongMenuBar::tr("Pitched voices");
    show_model(switch_table, switch_table.pitched_voices_model, new_row_type);
  } else if (new_row_type == RowType::unpitched_voice_type) {
    stream << SongMenuBar::tr("Unpitched voices");
    show_model(switch_table, switch_table.unpitched_voices_model, new_row_type);
  } else {
    auto& chord = chords[new_chord_number];
    if (new_row_type == RowType::pitched_note_type) {
      stream << SongMenuBar::tr("Pitched notes for chord ")
             << new_chord_number + 1;
      show_notes(switch_table, switch_table.pitched_notes_model,
                 chord.pitched_notes, new_row_type, row_type_changed,
                 new_chord_number, new_note_number);
    } else {
      Q_ASSERT(new_row_type == RowType::unpitched_note_type);
      stream << SongMenuBar::tr("Unpitched notes for chord ")
             << new_chord_number + 1;
      show_notes(switch_table, switch_table.unpitched_notes_model,
                 chord.unpitched_notes, new_row_type, row_type_changed,
                 new_chord_number, new_note_number);
    }
  }

  switch_column.editing_text.setText(label_text);

  switch_table.delegate.current_row_type = new_row_type;
  auto& selection_model = get_selection_model(switch_table);
  update_actions(song_menu_bar, window_body, selection_model);
  update_piano_roll_selection(piano_roll_widget, window_body);
  // set_model only swaps in a new selection model when row_type_changed, so
  // when it's false this reconnects to the same selection model as last
  // time; disconnect first so repeated calls (e.g. navigating between
  // chords of the same note type) don't pile up duplicate connections that
  // would each independently re-run update_actions/update_piano_roll_selection
  QObject::disconnect(&selection_model, &QItemSelectionModel::selectionChanged,
                      &selection_model, nullptr);
  QObject::connect(
      &selection_model, &QItemSelectionModel::selectionChanged,
      &selection_model,
      [&song_menu_bar, &window_body, &selection_model,
       &piano_roll_widget]() -> auto {
        update_actions(song_menu_bar, window_body, selection_model);
        update_piano_roll_selection(piano_roll_widget, window_body);
      });
}

auto ReplaceTable::id() const -> int { return REPLACE_TABLE_ID; }

auto ReplaceTable::mergeWith(const QUndoCommand* const next_command_pointer)
    -> bool {
  Q_ASSERT(next_command_pointer != nullptr);
  const auto& next_command =
      get_reference(dynamic_cast<const ReplaceTable*>(next_command_pointer));
  const auto next_row_type = next_command.new_row_type;
  const auto next_chord_number = next_command.new_chord_number;
  if (old_row_type == next_row_type && old_chord_number == next_chord_number) {
    setObsolete(true);
  }
  new_row_type = next_row_type;
  new_chord_number = next_chord_number;
  new_note_number = next_command.new_note_number;
  return true;
}

void ReplaceTable::undo() {
  replace_table(song_menu_bar, window_body, old_row_type, old_chord_number,
                piano_roll_widget);
}

void ReplaceTable::redo() {
  replace_table(song_menu_bar, window_body, new_row_type, new_chord_number,
                piano_roll_widget, new_note_number);
}
