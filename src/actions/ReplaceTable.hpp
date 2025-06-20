#pragma once

#include "menus/SongMenuBar.hpp"

static void switch_model(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                         const RowType row_type) {
  auto &switch_column = song_widget.switch_column;
  switch_column.current_row_type = row_type;
  switch_column.chords_table.setVisible(row_type == chord_type);
  switch_column.pitched_notes_table.setVisible(row_type == pitched_note_type);
  switch_column.unpitched_notes_table.setVisible(row_type ==
                                                 unpitched_note_type);
  update_actions(song_menu_bar, song_widget,
                 get_selection_model(get_table(switch_column)));
}

static void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          const RowType new_row_type,
                          const int new_chord_number) {
  auto &switch_column = song_widget.switch_column;
  auto &view_menu = song_menu_bar.view_menu;

  auto &previous_chord_action = view_menu.previous_chord_action;
  auto &next_chord_action = view_menu.next_chord_action;

  auto &chords = song_widget.song.chords;
  auto to_chords = new_row_type == chord_type;

  const auto old_row_type = switch_column.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  QString label_text;
  QTextStream stream(&label_text);
  switch (new_row_type) {
  case chord_type:
    stream << SongMenuBar::tr("Chords");
    break;
  case pitched_note_type:
    stream << SongMenuBar::tr("Pitched notes for chord ")
           << new_chord_number + 1;
    break;
  case unpitched_note_type:
    stream << SongMenuBar::tr("Unpitched notes for chord ")
           << new_chord_number + 1;
  }
  switch_column.editing_text.setText(label_text);

  if (row_type_changed) {
    song_menu_bar.view_menu.back_to_chords_action.setEnabled(!to_chords);
    song_menu_bar.file_menu.open_action.setEnabled(to_chords);
  }

  if (to_chords) {
    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);

    auto &chords_table = switch_column.chords_table;
    const auto chord_index = chords_table.chords_model.index(
        get_parent_chord_number(switch_column), 0);
    get_selection_model(chords_table)
        .select(chord_index, QItemSelectionModel::Select |
                                 QItemSelectionModel::Clear |
                                 QItemSelectionModel::Rows);
    chords_table.scrollTo(chord_index);

    switch_model(song_menu_bar, song_widget, new_row_type);
    if (old_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.pitched_notes_model.set_rows_pointer();
    } else {
      switch_column.unpitched_notes_table.unpitched_notes_model
          .set_rows_pointer();
    }
  } else {
    auto &chord = chords[new_chord_number];
    previous_chord_action.setEnabled(new_chord_number > 0);
    next_chord_action.setEnabled(new_chord_number < chords.size() - 1);
    if (new_row_type == pitched_note_type) {
      switch_column.pitched_notes_table.pitched_notes_model.set_rows_pointer(
          &chord.pitched_notes, new_chord_number);
    } else {
      switch_column.unpitched_notes_table.unpitched_notes_model
          .set_rows_pointer(&chord.unpitched_notes, new_chord_number);
    }
    switch_model(song_menu_bar, song_widget, new_row_type);
  }
}

struct ReplaceTable : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SongWidget &song_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;

  explicit ReplaceTable(SongMenuBar &song_menu_bar_input,
                        SongWidget &song_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input)
      : song_menu_bar(song_menu_bar_input), song_widget(song_widget_input),
        old_row_type(song_widget.switch_column.current_row_type),
        old_chord_number(get_parent_chord_number(song_widget.switch_column)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input){};

  [[nodiscard]] auto id() const -> int override { return replace_table_id; }

  [[nodiscard]] auto
  mergeWith(const QUndoCommand *const next_command_pointer) -> bool override {
    Q_ASSERT(next_command_pointer != nullptr);
    const auto &next_command =
        get_reference(dynamic_cast<const ReplaceTable *>(next_command_pointer));
    const auto next_row_type = next_command.new_row_type;
    const auto next_chord_number = next_command.new_chord_number;
    if (old_row_type == next_row_type &&
        old_chord_number == next_chord_number) {
      setObsolete(true);
    }
    new_row_type = next_row_type;
    new_chord_number = next_chord_number;
    return true;
  }

  void undo() override {
    replace_table(song_menu_bar, song_widget, old_row_type, old_chord_number);
  }

  void redo() override {
    replace_table(song_menu_bar, song_widget, new_row_type, new_chord_number);
  }
};
