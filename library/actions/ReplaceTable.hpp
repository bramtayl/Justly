#pragma once

#include <algorithm>

#include "cell_editors/IntervalEditor.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/SongMenuBar.hpp"
#include "rows/RowType.hpp"

static auto get_string_picker_width(const QList<QString> &names) {
  StringPicker editor(nullptr, names);
  editor.setFrame(false);
  return editor.sizeHint().width();
}

static auto set_minimum_column_size(QTableView &view, const int column_number,
                                    const int minimum_size) {
  view.resizeColumnToContents(column_number);
  view.setColumnWidth(
      column_number, std::max({minimum_size, view.columnWidth(column_number)}));
}

static auto get_is_voice(const RowType row_type) -> bool {
  return row_type == pitched_voice_type || row_type == unpitched_voice_type;
}

static auto get_voice_name_column(const RowType row_type) -> int {
  return row_type == pitched_voice_type
             ? static_cast<int>(pitched_voice_name_column)
             : static_cast<int>(unpitched_voice_name_column);
}

static void update_actions(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                           const QItemSelectionModel &selector) {
  auto &edit_menu = song_menu_bar.edit_menu;
  auto &controls_column = song_widget.controls_column;

  const auto selection = selector.selection();

  const auto anything_selected = !selection.empty();

  const auto& switch_table = song_widget.switch_column.switch_table;

  const auto current_row_type = switch_table.delegate.current_row_type;
  const auto is_voice = get_is_voice(current_row_type);

  set_interval_rows_is_enabled(
      controls_column.third_row, controls_column.fifth_row,
      controls_column.seventh_row, controls_column.octave_row,
      anything_selected &&
          !is_voice &&
          current_row_type !=
              unpitched_note_type);

  song_menu_bar.play_menu.play_action.setEnabled(anything_selected);

  // voice names must be typed, not copy/pasted, since every voice name must
  // stay unique and non-empty
  const auto name_column_selected =
      is_voice &&
      std::ranges::any_of(
          selection, [name_column = get_voice_name_column(current_row_type)](
                        const QItemSelectionRange &range) {
            return range.left() <= name_column && name_column <= range.right();
          });
  const auto can_copy_paste = anything_selected && !name_column_selected;

  edit_menu.cut_action.setEnabled(can_copy_paste);
  edit_menu.copy_action.setEnabled(can_copy_paste);
  auto& paste_menu = edit_menu.paste_menu;
  paste_menu.paste_over_action.setEnabled(can_copy_paste);
  // pasting after/into always inserts a brand new row built only from the
  // pasted column(s), so for voices it would create one with an empty
  // (invalid) name -- unlike paste_over, which only ever touches existing,
  // already-named rows
  paste_menu.paste_after_action.setEnabled(can_copy_paste && !is_voice);
  paste_menu.paste_into_start_action.setEnabled(!is_voice);
  edit_menu.delete_cells_action.setEnabled(anything_selected);
  edit_menu.remove_rows_action.setEnabled(anything_selected);

  auto& insert_after_action = edit_menu.insert_menu.insert_after_action;
  if (is_voice) {
    // voices can only be appended after the last row, not inserted in the
    // middle
    auto is_last_selected = false;
    const auto row_count = get_reference(switch_table.model()).rowCount();
    for (const auto& range : selection) {
      if (range.bottom() == row_count - 1) {
        is_last_selected = true;
        break;
      }
    }
    insert_after_action.setEnabled(is_last_selected);
  } else {
    insert_after_action.setEnabled(anything_selected);
  }
}

static void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          const RowType new_row_type,
                          const int new_chord_number,
                          const int new_note_number = -1) {
  auto &song = song_widget.song;
  auto &switch_column = song_widget.switch_column;
  auto &switch_table = switch_column.switch_table;
  auto &view_menu = song_menu_bar.view_menu;

  auto &previous_chord_action = view_menu.previous_chord_action;
  auto &next_chord_action = view_menu.next_chord_action;

  auto &chords = song_widget.song.chords;
  auto to_chords = new_row_type == chord_type;

  const auto old_row_type = switch_table.delegate.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  const auto interval_width = get_minimum_size<IntervalEditor>().width();
  const auto rational_width = get_minimum_size<RationalEditor>().width();
  static const auto instrument_width =
      get_string_picker_width(get_some_program_names(true));
  static const auto percussion_set_width =
      get_string_picker_width(get_some_program_names(false));

  QString label_text;
  QTextStream stream(&label_text);

  if (new_row_type == chord_type) {
    stream << SongMenuBar::tr("Chords");

    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);
    const auto old_parent_chord_number = get_parent_chord_number(switch_table);

    set_model(switch_table, switch_table.chords_model);

    set_minimum_column_size(switch_table, chord_interval_column,
                            interval_width);
    set_minimum_column_size(switch_table, chord_beats_column, rational_width);
    set_minimum_column_size(switch_table, chord_velocity_ratio_column,
                            rational_width);
    set_minimum_column_size(switch_table, chord_tempo_ratio_column,
                            rational_width);
    switch_table.resizeColumnToContents(chord_pitched_notes_column);
    switch_table.resizeColumnToContents(chord_unpitched_notes_column);
    set_minimum_column_size(switch_table, chord_words_column, WORDS_WIDTH);

    if (old_parent_chord_number >= 0) {
      const auto chord_index =
          switch_table.chords_model.index(old_parent_chord_number, 0);
      get_selection_model(switch_table)
          .select(chord_index, QItemSelectionModel::Select |
                                   QItemSelectionModel::Clear |
                                   QItemSelectionModel::Rows);
      switch_table.scrollTo(chord_index);
    }

    switch (old_row_type) {
    case chord_type:
    case pitched_voice_type:
    case unpitched_voice_type:
      break;
    case pitched_note_type:
      switch_table.pitched_notes_model.set_rows_pointer();
      break;
    case unpitched_note_type:
      switch_table.unpitched_notes_model.set_rows_pointer();
      break;
    }
  } else if (new_row_type == pitched_voice_type) {
    stream << SongMenuBar::tr("Pitched voices");
    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);
    set_model(switch_table, switch_table.pitched_voices_model);
    set_minimum_column_size(switch_table, pitched_voice_instrument_column,
                            instrument_width);
    set_minimum_column_size(switch_table, pitched_voice_velocity_ratio_column,
                            rational_width);
    set_minimum_column_size(switch_table, pitched_voice_name_column,
                            WORDS_WIDTH);
  } else if (new_row_type == unpitched_voice_type) {
    stream << SongMenuBar::tr("Unpitched voices");
    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);
    set_model(switch_table, switch_table.unpitched_voices_model);
    set_minimum_column_size(switch_table, unpitched_voice_percussion_set_column,
                            percussion_set_width);
    switch_table.resizeColumnToContents(unpitched_voice_midi_number_column);
    set_minimum_column_size(switch_table, unpitched_voice_velocity_ratio_column,
                            rational_width);
    set_minimum_column_size(switch_table, unpitched_voice_name_column,
                            WORDS_WIDTH);
  } else {
    auto &chord = chords[new_chord_number];
    previous_chord_action.setEnabled(new_chord_number > 0);
    next_chord_action.setEnabled(new_chord_number < chords.size() - 1);
    if (new_row_type == pitched_note_type) {
      auto &new_model = switch_table.pitched_notes_model;
      stream << SongMenuBar::tr("Pitched notes for chord ")
             << new_chord_number + 1;
      new_model.set_rows_pointer(&chord.pitched_notes, new_chord_number);
      if (row_type_changed) {
        set_model(switch_table, new_model);

        set_minimum_column_size(
            switch_table, pitched_note_voice_number_column,
            get_string_picker_width(get_names(song.pitched_voices)));
        set_minimum_column_size(switch_table, pitched_note_interval_column,
                                interval_width);
        set_minimum_column_size(switch_table, pitched_note_beats_column,
                                rational_width);
        set_minimum_column_size(
            switch_table, pitched_note_velocity_ratio_column, rational_width);
        set_minimum_column_size(switch_table, pitched_note_words_column,
                                WORDS_WIDTH);
      }
      if (new_note_number >= 0) {
        const auto note_index = new_model.index(new_note_number, 0);
        get_selection_model(switch_table)
            .select(note_index, QItemSelectionModel::Select |
                                    QItemSelectionModel::Clear |
                                    QItemSelectionModel::Rows);
        switch_table.scrollTo(note_index);
      }
    } else if (new_row_type == unpitched_note_type) {
      auto &new_model = switch_table.unpitched_notes_model;
      stream << SongMenuBar::tr("Unpitched notes for chord ")
             << new_chord_number + 1;
      new_model.set_rows_pointer(&chord.unpitched_notes, new_chord_number);
      if (row_type_changed) {
        set_model(switch_table, new_model);

        set_minimum_column_size(
            switch_table, unpitched_note_voice_number_column,
            get_string_picker_width(get_names(song.unpitched_voices)));
        set_minimum_column_size(switch_table, unpitched_note_beats_column,
                                rational_width);
        set_minimum_column_size(
            switch_table, unpitched_note_velocity_ratio_column, rational_width);
        set_minimum_column_size(switch_table, unpitched_note_words_column,
                                WORDS_WIDTH);
      }
      if (new_note_number >= 0) {
        const auto note_index = new_model.index(new_note_number, 0);
        get_selection_model(switch_table)
            .select(note_index, QItemSelectionModel::Select |
                                    QItemSelectionModel::Clear |
                                    QItemSelectionModel::Rows);
        switch_table.scrollTo(note_index);
      }
    }
  }

  switch_column.editing_text.setText(label_text);

  const auto can_edit_voices = !get_is_voice(new_row_type);

  song_menu_bar.view_menu.back_to_chords_action.setEnabled(!to_chords);
  song_menu_bar.view_menu.edit_pitched_voices_action.setEnabled(can_edit_voices);
  song_menu_bar.view_menu.edit_unpitched_voices_action.setEnabled(can_edit_voices);
  song_menu_bar.file_menu.open_action.setEnabled(to_chords);

  switch_table.delegate.current_row_type = new_row_type;
  auto &selection_model = get_selection_model(switch_table);
  update_actions(song_menu_bar, song_widget, selection_model);
  QObject::connect(
      &selection_model, &QItemSelectionModel::selectionChanged,
      &selection_model, [&song_menu_bar, &song_widget, &selection_model]() {
        update_actions(song_menu_bar, song_widget, selection_model);
      });
}

struct ReplaceTable : public QUndoCommand {
  SongMenuBar &song_menu_bar;
  SongWidget &song_widget;
  const RowType old_row_type;
  const int old_chord_number;
  RowType new_row_type;
  int new_chord_number;
  int new_note_number;

  explicit ReplaceTable(SongMenuBar &song_menu_bar_input,
                        SongWidget &song_widget_input,
                        const RowType new_row_type_input,
                        const int new_chord_number_input,
                        const int new_note_number_input = -1)
      : song_menu_bar(song_menu_bar_input), song_widget(song_widget_input),
        old_row_type(
            song_widget.switch_column.switch_table.delegate.current_row_type),
        old_chord_number(
            get_parent_chord_number(song_widget.switch_column.switch_table)),
        new_row_type(new_row_type_input),
        new_chord_number(new_chord_number_input),
        new_note_number(new_note_number_input){};

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
    new_note_number = next_command.new_note_number;
    return true;
  }

  void undo() override {
    replace_table(song_menu_bar, song_widget, old_row_type, old_chord_number);
  }

  void redo() override {
    replace_table(song_menu_bar, song_widget, new_row_type, new_chord_number,
                 new_note_number);
  }
};
