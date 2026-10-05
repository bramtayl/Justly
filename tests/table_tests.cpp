#include <QtWidgets/QSpinBox>

#include "Tester.hpp"
#include "cell_editors/RationalEditor.hpp"
#include "cell_editors/StringPicker.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"

void Tester::test_column_count_data() {
  add_table_columns();
  QTest::addColumn<int>("number_of_columns");

  QTest::newRow("chord") << RowType::chord_type << -1
                         << static_cast<int>(
                                ChordColumn::number_of_chord_columns);
  QTest::newRow("pitched note")
      << RowType::pitched_note_type << 0
      << static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 0
      << static_cast<int>(
             UnpitchedNoteColumn::number_of_unpitched_note_columns);
  QTest::newRow("pitched voice")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::number_of_pitched_voice_columns);
  QTest::newRow("unpitched voice")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(
             UnpitchedVoiceColumn::number_of_unpitched_voice_columns);
}

void Tester::test_column_count() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, number_of_columns);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  switch_to(main_window, row_type, chord_number);
  QCOMPARE(get_model(switch_table).columnCount(), number_of_columns);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

void Tester::test_column_header_data() {
  add_table_columns();
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("column_name");

  QTest::newRow("chord interval")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_interval_column) << "Interval";
  QTest::newRow("chord beats")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_beats_column) << "Beats";
  QTest::newRow("chord velocity ratio")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("chord tempo ratio")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_tempo_ratio_column)
      << "Tempo ratio";
  QTest::newRow("chord words")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_words_column) << "Words";
  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_pitched_notes_column)
      << "Pitched notes";
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_unpitched_notes_column)
      << "Unpitched notes";
  QTest::newRow("pitched note voice")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_name_column)
      << "Voice";
  QTest::newRow("pitched note interval")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_interval_column)
      << "Interval";
  QTest::newRow("pitched note beats")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_beats_column)
      << "Beats";
  QTest::newRow("pitched note velocity ratio")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("pitched note words")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_words_column)
      << "Words";
  QTest::newRow("unpitched note voice")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_name_column)
      << "Voice";
  QTest::newRow("unpitched note beats")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column)
      << "Beats";
  QTest::newRow("unpitched note velocity ratio")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(
             UnpitchedNoteColumn::unpitched_note_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("unpitched note words")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column)
      << "Words";
  QTest::newRow("pitched voice name")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
      << "Name";
  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column)
      << "Instrument";
  QTest::newRow("pitched voice velocity ratio")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(
             PitchedVoiceColumn::pitched_voice_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("unpitched voice name")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
      << "Name";
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_percussion_set_column)
      << "Percussion set";
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_midi_number_column)
      << "MIDI number";
  QTest::newRow("unpitched voice velocity ratio")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column)
      << "Velocity ratio";
}

void Tester::test_column_header() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, column_number);
  QFETCH(const QString, column_name);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  switch_to(main_window, row_type, chord_number);
  QCOMPARE(get_model(switch_table).headerData(column_number, Qt::Horizontal),
           column_name);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

void Tester::test_copy_data() {
  add_cell_pairs();
  add_voice_column_pairs();
}

void Tester::test_copy() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, first_row_number);
  QFETCH(const int, second_row_number);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  auto& selector = get_selection_model(switch_table);

  const auto& first_index = model.index(first_row_number, column_number);
  const auto& second_index = model.index(second_row_number, column_number);

  const auto first_value = first_index.data();
  const auto second_value = second_index.data();

  QCOMPARE_NE(first_value, second_value);

  selector.select(first_index, SELECT_AND_CLEAR);
  edit_menu.copy_action.trigger();

  selector.select(second_index, SELECT_AND_CLEAR);
  edit_menu.paste_menu.paste_over_action.trigger();

  QCOMPARE(second_index.data(), first_value);
  undo_stack.undo();
  QCOMPARE(second_index.data(), second_value);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_cut_data() { add_cell_pairs(); }

void Tester::test_cut() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, first_row_number);
  QFETCH(const int, second_row_number);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  auto& selector = get_selection_model(switch_table);

  const auto& first_index = model.index(first_row_number, column_number);
  const auto& second_index = model.index(second_row_number, column_number);

  const auto first_value = first_index.data();
  const auto second_value = second_index.data();

  QCOMPARE_NE(first_value, second_value);

  selector.select(second_index, SELECT_AND_CLEAR);
  edit_menu.cut_action.trigger();

  QCOMPARE(second_index.data(), first_value);

  selector.select(first_index, SELECT_AND_CLEAR);
  edit_menu.paste_menu.paste_over_action.trigger();

  QCOMPARE(first_index.data(), second_value);
  undo_stack.undo();
  QCOMPARE(first_index.data(), first_value);
  undo_stack.undo();
  QCOMPARE(second_index.data(), second_value);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_delete_data() {
  add_cells();
  // voice names can't be deleted; see test_voice_name_delete_disabled
  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type << -1 << 0
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type << -1 << 0
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type << -1 << 0
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
}

void Tester::test_delete() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  switch_to(main_window, row_type, chord_number);

  const auto delete_index =
      get_model(switch_table).index(row_number, column_number);
  const auto& old_value = delete_index.data();

  get_selection_model(switch_table).select(delete_index, SELECT_AND_CLEAR);
  main_window.song_menu_bar.edit_menu.delete_cells_action.trigger();

  QCOMPARE_NE(old_value, delete_index.data());
  undo_stack.undo();
  QCOMPARE(old_value, delete_index.data());

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_next_previous_data() {
  add_table_columns();

  QTest::newRow("pitched note") << RowType::pitched_note_type << 1;
  QTest::newRow("unpitched note") << RowType::unpitched_note_type << 1;
}

void Tester::test_next_previous() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& view_menu = main_window.song_menu_bar.view_menu;

  switch_to(main_window, row_type, chord_number);
  QCOMPARE(get_parent_chord_number(switch_table), chord_number);
  view_menu.previous_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_table), chord_number - 1);
  view_menu.next_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_table), chord_number);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

// replace_table() (ReplaceTable.hpp) reconnects an update lambda to the
// switch table's selection model on every call, but only swaps in a fresh
// selection model (via set_model) when the row type actually changes --
// when it doesn't (e.g. next/previous_chord_action, exercised above),
// the same QItemSelectionModel is reused across calls, so a bare
// connect() without a preceding disconnect() piles up one duplicate
// connection per call. This reproduces that exact connect/reconnect
// pattern against the real selection model object used by the switch
// table, using a private receiver so it can't disturb replace_table's own
// connection to that same model.
void Tester::
    test_reconnecting_selection_model_does_not_duplicate_connections() {
  auto& switch_table = main_window.window_body.switch_column.switch_table;
  auto& selection_model = get_selection_model(switch_table);

  // control: connecting without disconnecting first (the pre-fix pattern)
  // really does accumulate one call per reconnect
  {
    select_cell(switch_table, 0, 1);
    const QObject receiver;
    auto call_count = 0;
    for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
      QObject::connect(
          &selection_model, &QItemSelectionModel::selectionChanged, &receiver,
          [&call_count]() -> auto { call_count = call_count + 1; });
    }
    select_cell(switch_table, 0, 0);
    QCOMPARE(call_count, 5);
  }

  // the fix: disconnecting before reconnecting keeps exactly one
  // connection alive no matter how many times replace_table() re-runs
  // against the same selection model
  {
    select_cell(switch_table, 0, 1);
    const QObject receiver;
    auto call_count = 0;
    for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
      QObject::disconnect(&selection_model,
                          &QItemSelectionModel::selectionChanged, &receiver,
                          nullptr);
      QObject::connect(
          &selection_model, &QItemSelectionModel::selectionChanged, &receiver,
          [&call_count]() -> auto { call_count = call_count + 1; });
    }
    select_cell(switch_table, 0, 0);
    QCOMPARE(call_count, 1);
  }
}

void Tester::test_replace_table_combining() {
  auto& window_body = main_window.window_body;
  switch_to(main_window, RowType::unpitched_note_type, 0);
  main_window.song_menu_bar.view_menu.back_to_chords_action.trigger();
  QVERIFY(!window_body.undo_stack.canUndo());
}

void Tester::test_row_count_data() {
  add_table_columns();
  QTest::addColumn<int>("number_of_rows");

  QTest::newRow("chords") << RowType::chord_type << -1 << EIGHT;
  QTest::newRow("chord 0 pitched notes")
      << RowType::pitched_note_type << 0 << 0;
  QTest::newRow("chord 1 pitched notes")
      << RowType::pitched_note_type << 1 << EIGHT;
  QTest::newRow("chord 2 pitched notes")
      << RowType::pitched_note_type << 2 << 0;
  QTest::newRow("chord 3 pitched notes")
      << RowType::pitched_note_type << 3 << 0;
  QTest::newRow("chord 4 pitched notes")
      << RowType::pitched_note_type << 4 << 0;
  QTest::newRow("chord 5 pitched notes")
      << RowType::pitched_note_type << FIVE << 0;
  QTest::newRow("chord 6 pitched notes")
      << RowType::pitched_note_type << SIX << 0;
  QTest::newRow("chord 7 pitched notes")
      << RowType::pitched_note_type << SEVEN << 0;
  QTest::newRow("chord 0 unpitched notes")
      << RowType::unpitched_note_type << 0 << 0;
  QTest::newRow("chord 1 unpitched notes")
      << RowType::unpitched_note_type << 1 << 4;
  QTest::newRow("chord 2 unpitched notes")
      << RowType::unpitched_note_type << 2 << 0;
  QTest::newRow("chord 3 unpitched notes")
      << RowType::unpitched_note_type << 3 << 0;
  QTest::newRow("chord 4 unpitched notes")
      << RowType::unpitched_note_type << 4 << 0;
  QTest::newRow("chord 5 unpitched notes")
      << RowType::unpitched_note_type << FIVE << 0;
  QTest::newRow("chord 6 unpitched notes")
      << RowType::unpitched_note_type << SIX << 0;
  QTest::newRow("chord 7 unpitched notes")
      << RowType::unpitched_note_type << SEVEN << 0;
  QTest::newRow("pitched voices") << RowType::pitched_voice_type << -1 << 2;
  QTest::newRow("unpitched voices") << RowType::unpitched_voice_type << -1 << 3;
}

void Tester::test_row_count() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, number_of_rows);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  switch_to(main_window, row_type, chord_number);
  QCOMPARE(get_model(switch_table).rowCount(), number_of_rows);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

void Tester::test_row_header_data() {
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("data");

  QTest::newRow("text") << Qt::DisplayRole << QVariant(1);
  QTest::newRow("unused role") << Qt::DecorationRole << QVariant();
}

void Tester::test_row_header() {
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, data);

  QCOMPARE(get_model(main_window.window_body.switch_column.switch_table)
               .headerData(0, Qt::Vertical, role),
           data);
}

void Tester::test_starting_control_data() {
  QTest::addColumn<QDoubleSpinBox*>("spin_box_pointer");
  QTest::addColumn<double*>("value_pointer");
  QTest::addColumn<double>("first_value");
  QTest::addColumn<double>("second_value");

  auto& window_body = main_window.window_body;
  auto& song = window_body.song;
  auto& spin_boxes = window_body.controls_column.spin_boxes;

  QTest::newRow("key") << &spin_boxes.starting_key_editor << &song.starting_key
                       << STARTING_KEY_1 << STARTING_KEY_2;
  QTest::newRow("velocity")
      << &spin_boxes.starting_velocity_editor << &song.starting_velocity
      << STARTING_VELOCITY_1 << STARTING_VELOCITY_2;
  QTest::newRow("tempo") << &spin_boxes.starting_tempo_editor
                         << &song.starting_tempo << STARTING_TEMPO_1
                         << STARTING_TEMPO_2;
}

void Tester::test_starting_control() {
  QFETCH(QDoubleSpinBox*, spin_box_pointer);
  QFETCH(double*, value_pointer);
  QFETCH(const double, first_value);
  QFETCH(const double, second_value);

  auto& spin_box = get_reference(spin_box_pointer);
  auto& value = get_reference(value_pointer);

  const auto old_value = value;
  QCOMPARE_NE(old_value, first_value);
  QCOMPARE_NE(old_value, second_value);

  // test combining
  spin_box.setValue(first_value);
  QCOMPARE(value, first_value);
  spin_box.setValue(second_value);
  QCOMPARE(value, second_value);
  main_window.window_body.undo_stack.undo();
  QCOMPARE(value, old_value);
}

void Tester::test_status_data() {
  add_table_columns();
  QTest::addColumn<QString>("status");

  QTest::newRow("pitched note") << RowType::pitched_note_type << 1
                                << "660 Hz ≈ E5 + 2 cents; Velocity 30; 300 "
                                   "bpm; Start at 600 ms; Duration 200 ms";
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 1
      << "Velocity 30; 300 bpm; Start at 600 ms; Duration 200 ms";
  // voice rows have no status tip; exercises RowsModel's default
  // add_to_status
  QTest::newRow("pitched voice") << RowType::pitched_voice_type << -1 << "";
  QTest::newRow("unpitched voice") << RowType::unpitched_voice_type << -1 << "";
}

void Tester::test_status() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const QString, status);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  switch_to(main_window, row_type, chord_number);
  QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole), status);
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

void Tester::test_set_value_data() {
  add_editable_cell_pairs();
  add_voice_column_pairs();
}

void Tester::test_set_value() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, first_row_number);
  QFETCH(const int, second_row_number);
  QFETCH(const int, column_number);

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  const auto second_index = model.index(second_row_number, column_number);

  // use EditRole rather than the default DisplayRole: for the voice
  // columns, DisplayRole shows the voice's name while EditRole (what the
  // cell editor actually reads/writes) is the underlying voice number, so
  // comparing/round-tripping DisplayRole values would bypass the editor
  const auto first_value =
      model.index(first_row_number, column_number).data(Qt::EditRole);
  const auto second_value = second_index.data(Qt::EditRole);
  QCOMPARE_NE(first_value, second_value);

  auto& delegate = get_reference(switch_table.itemDelegate());
  auto& cell_editor = get_reference(
      delegate.createEditor(&get_reference(switch_table.viewport()),
                            QStyleOptionViewItem(), second_index));
  delegate.setEditorData(&cell_editor, second_index);
  cell_editor.setProperty(
      get_reference(cell_editor.metaObject()).userProperty().name(),
      first_value);
  delegate.setModelData(&cell_editor, &model, second_index);

  QCOMPARE(second_index.data(Qt::EditRole), first_value);
  undo_stack.undo();
  QCOMPARE(second_index.data(Qt::EditRole), second_value);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_to_string_data() {
  QTest::addColumn<int>("row_number");
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("text");

  QTest::newRow("chord 0 interval")
      << 0 << static_cast<int>(ChordColumn::chord_interval_column) << "";
  QTest::newRow("chord 1 interval")
      << 1 << static_cast<int>(ChordColumn::chord_interval_column) << "3";
  QTest::newRow("chord 2 interval")
      << 2 << static_cast<int>(ChordColumn::chord_interval_column) << "/5";
  QTest::newRow("chord 3 interval")
      << 3 << static_cast<int>(ChordColumn::chord_interval_column) << "3/5";
  QTest::newRow("chord 4 interval")
      << 4 << static_cast<int>(ChordColumn::chord_interval_column) << "o1";
  QTest::newRow("chord 5 interval")
      << FIVE << static_cast<int>(ChordColumn::chord_interval_column) << "3o1";
  QTest::newRow("chord 6 interval")
      << SIX << static_cast<int>(ChordColumn::chord_interval_column) << "/5o1";
  QTest::newRow("chord 7 interval")
      << SEVEN << static_cast<int>(ChordColumn::chord_interval_column)
      << "3/5o1";
  QTest::newRow("chord 0 beats")
      << 0 << static_cast<int>(ChordColumn::chord_beats_column) << "";
  QTest::newRow("chord 1 beats")
      << 1 << static_cast<int>(ChordColumn::chord_beats_column) << "3";
  QTest::newRow("chord 2 beats")
      << 2 << static_cast<int>(ChordColumn::chord_beats_column) << "/5";
  QTest::newRow("chord 3 beats")
      << 3 << static_cast<int>(ChordColumn::chord_beats_column) << "3/5";
}

void Tester::test_to_string() {
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);
  QFETCH(const QString, text);

  QCOMPARE(get_model(main_window.window_body.switch_column.switch_table)
               .index(row_number, column_number)
               .data()
               .toString(),
           text);
}

void Tester::test_unused_role_data() { add_tables(); }

void Tester::test_unused_role() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  switch_to(main_window, row_type, chord_number);
  auto& model = get_model(switch_table);
  const auto test_index = model.index(0, 0);
  QCOMPARE(test_index.data(Qt::DecorationRole), QVariant());
  QVERIFY(!(model.setData(test_index, QVariant(), Qt::DecorationRole)));
  maybe_switch_back_to_chords(window_body.undo_stack, row_type);
}

void Tester::test_voice_cell_editors_data() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("editor_kind");

  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column)
      << "string picker";
  QTest::newRow("pitched voice velocity ratio")
      << RowType::pitched_voice_type
      << static_cast<int>(
             PitchedVoiceColumn::pitched_voice_velocity_ratio_column)
      << "rational";
  QTest::newRow("pitched voice name")
      << RowType::pitched_voice_type
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
      << "text";
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_percussion_set_column)
      << "string picker";
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_midi_number_column)
      << "midi number";
  QTest::newRow("unpitched voice velocity ratio")
      << RowType::unpitched_voice_type
      << static_cast<int>(
             UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column)
      << "rational";
  QTest::newRow("unpitched voice name")
      << RowType::unpitched_voice_type
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
      << "text";
}

// each voice column gets the editor suited to its values
void Tester::test_voice_cell_editors() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, column_number);
  QFETCH(const QString, editor_kind);

  auto& switch_table = main_window.window_body.switch_column.switch_table;
  switch_to(main_window, row_type, -1);

  auto& delegate = get_reference(switch_table.itemDelegate());
  auto* const editor_pointer = delegate.createEditor(
      &get_reference(switch_table.viewport()), QStyleOptionViewItem(),
      get_model(switch_table).index(0, column_number));
  const auto actual_kind = [editor_pointer]() -> QString {
    if (dynamic_cast<StringPicker*>(editor_pointer) != nullptr) {
      return "string picker";
    }
    if (dynamic_cast<RationalEditor*>(editor_pointer) != nullptr) {
      return "rational";
    }
    const auto* const spin_box_pointer =
        dynamic_cast<QSpinBox*>(editor_pointer);
    if (spin_box_pointer != nullptr && spin_box_pointer->minimum() == 0 &&
        spin_box_pointer->maximum() ==
            127) {  // NOLINT(readability-magic-numbers)
      return "midi number";
    }
    if (dynamic_cast<QLineEdit*>(editor_pointer) != nullptr) {
      return "text";
    }
    return "other";
  }();
  delete editor_pointer;  // NOLINT(cppcoreguidelines-owning-memory)
  QCOMPARE(actual_kind, editor_kind);

  maybe_switch_back_to_chords(main_window.window_body.undo_stack, row_type);
}

// moving between chords' notes after an edit merges the moves into one undo
// step, like it does without the edit
void Tester::test_navigate_chords_after_edit() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& view_menu = main_window.song_menu_bar.view_menu;

  switch_to(main_window, RowType::pitched_note_type, 1);
  auto& model = get_model(switch_table);
  QVERIFY(model.setData(
      model.index(
          0, static_cast<int>(PitchedNoteColumn::pitched_note_words_column)),
      "edited"));
  const auto index_after_edit = undo_stack.index();

  view_menu.next_chord_action.trigger();
  view_menu.next_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_table), 3);
  QCOMPARE(undo_stack.index(), index_after_edit + 1);

  undo_stack.undo();
  QCOMPARE(switch_table.delegate.current_row_type, RowType::pitched_note_type);
  QCOMPARE(get_parent_chord_number(switch_table), 1);

  undo_stack.undo();  // the edit
  undo_stack.undo();  // back to chords
  QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);
}

// double-clicking only opens notes from a chord's notes columns
void Tester::test_double_click_outside_notes_columns() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  double_click_cell(switch_table, 1,
                    static_cast<int>(ChordColumn::chord_interval_column));
  QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);

  switch_to(main_window, RowType::pitched_note_type, 1);
  double_click_cell(switch_table, 0, 0);
  QCOMPARE(switch_table.delegate.current_row_type, RowType::pitched_note_type);
  QCOMPARE(get_parent_chord_number(switch_table), 1);
  maybe_switch_back_to_chords(window_body.undo_stack,
                              RowType::pitched_note_type);
}
