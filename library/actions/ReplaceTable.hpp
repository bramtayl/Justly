#pragma once

#include "cell_editors/IntervalEditor.hpp"
#include "cell_editors/PercussionInstrumentEditor.hpp"
#include "cell_editors/ProgramEditor.hpp"
#include "menus/SongMenuBar.hpp"

static void replace_table(SongMenuBar &song_menu_bar, SongWidget &song_widget,
                          const RowType new_row_type,
                          const int new_chord_number) {
  auto &switch_column = song_widget.switch_column;
  auto &switch_table = switch_column.switch_table;
  auto &view_menu = song_menu_bar.view_menu;

  auto &previous_chord_action = view_menu.previous_chord_action;
  auto &next_chord_action = view_menu.next_chord_action;

  auto &chords = song_widget.song.chords;
  auto to_chords = new_row_type == chord_type;

  const auto old_row_type = switch_table.current_row_type;
  const auto row_type_changed = old_row_type != new_row_type;

  const auto interval_width = get_minimum_size<IntervalEditor>().width();
  const auto instrument_width = get_minimum_size<ProgramEditor>().width();
  const auto percussion_instrument_width =
      get_minimum_size<PercussionInstrumentEditor>().width();
  const auto rational_width = get_minimum_size<RationalEditor>().width();

  QString label_text;
  QTextStream stream(&label_text);

  if (to_chords) {
    stream << SongMenuBar::tr("Chords");

    previous_chord_action.setEnabled(false);
    next_chord_action.setEnabled(false);
    const auto chord_number = get_parent_chord_number(switch_table);

    set_model(switch_table, switch_table.chords_model);

    switch_table.setColumnWidth(chord_instrument_column, instrument_width);
    switch_table.setColumnWidth(chord_percussion_instrument_column,
                                percussion_instrument_width);
    switch_table.setColumnWidth(chord_interval_column, interval_width);
    switch_table.setColumnWidth(chord_beats_column, rational_width);
    switch_table.setColumnWidth(chord_velocity_ratio_column, rational_width);
    switch_table.setColumnWidth(chord_tempo_ratio_column, rational_width);
    switch_table.resizeColumnToContents(chord_pitched_notes_column);
    switch_table.resizeColumnToContents(chord_unpitched_notes_column);
    switch_table.setColumnWidth(chord_words_column, WORDS_WIDTH);

    if (new_chord_number >= 0) {
      const auto chord_index = switch_table.chords_model.index(chord_number, 0);
      get_selection_model(switch_table)
          .select(chord_index, QItemSelectionModel::Select |
                                   QItemSelectionModel::Clear |
                                   QItemSelectionModel::Rows);
      switch_table.scrollTo(chord_index);
    }

    switch (old_row_type) {
    case chord_type:
      break;
    case pitched_note_type:
      switch_table.pitched_notes_model.set_rows_pointer();
      break;
    case unpitched_note_type:
      switch_table.unpitched_notes_model.set_rows_pointer();
      break;
    default:
      Q_ASSERT(false);
    }
  } else {
    auto &chord = chords[new_chord_number];
    previous_chord_action.setEnabled(new_chord_number > 0);
    next_chord_action.setEnabled(new_chord_number < chords.size() - 1);
    switch (new_row_type) {
    case chord_type:
      break;
    case pitched_note_type:
      stream << SongMenuBar::tr("Pitched notes for chord ")
             << new_chord_number + 1;
      switch_table.pitched_notes_model.set_rows_pointer(&chord.pitched_notes,
                                                        new_chord_number);
      if (row_type_changed) {
        set_model(switch_table, switch_table.pitched_notes_model);

        switch_table.setColumnWidth(pitched_note_instrument_column,
                                    instrument_width);
        switch_table.setColumnWidth(pitched_note_interval_column,
                                    interval_width);
        switch_table.setColumnWidth(pitched_note_beats_column, rational_width);
        switch_table.setColumnWidth(pitched_note_velocity_ratio_column,
                                    rational_width);
        switch_table.setColumnWidth(pitched_note_words_column, WORDS_WIDTH);
      }
      break;
    case unpitched_note_type:
      stream << SongMenuBar::tr("Unpitched notes for chord ")
             << new_chord_number + 1;
      switch_table.unpitched_notes_model.set_rows_pointer(
          &chord.unpitched_notes, new_chord_number);
      if (row_type_changed) {
        set_model(switch_table, switch_table.unpitched_notes_model);

        switch_table.setColumnWidth(unpitched_note_percussion_instrument_column,
                                    percussion_instrument_width);
        switch_table.setColumnWidth(unpitched_note_beats_column,
                                    rational_width);
        switch_table.setColumnWidth(unpitched_note_velocity_ratio_column,
                                    rational_width);
        switch_table.setColumnWidth(unpitched_note_words_column, WORDS_WIDTH);
      }
      break;
    default:
      Q_ASSERT(false);
    }
  }

  switch_column.editing_text.setText(label_text);

  if (row_type_changed) {
    song_menu_bar.view_menu.back_to_chords_action.setEnabled(!to_chords);
    song_menu_bar.file_menu.open_action.setEnabled(to_chords);

    switch_table.current_row_type = new_row_type;
    auto &selection_model = get_selection_model(switch_table);
    update_actions(song_menu_bar, song_widget, selection_model);
    QObject::connect(
        &selection_model, &QItemSelectionModel::selectionChanged,
        &selection_model, [&song_menu_bar, &song_widget, &selection_model]() {
          update_actions(song_menu_bar, song_widget, selection_model);
        });
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
        old_row_type(song_widget.switch_column.switch_table.current_row_type),
        old_chord_number(
            get_parent_chord_number(song_widget.switch_column.switch_table)),
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
