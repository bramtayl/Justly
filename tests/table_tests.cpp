#include <QtCore/QAbstractItemModel>
#include <QtCore/QDebug>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/qobjectdefs.h>
#include <QtGui/QAction>
#include <QtGui/QUndoStack>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>

#include "Tester.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/EditMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "test_helpers.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

void Tester::test_column_count_data() {
  add_table_columns();
  QTest::addColumn<int>("number_of_columns");

  QTest::newRow("chord") << RowType::chord_type << -1
                         << static_cast<int>(ChordColumn::number_of_chord_columns);
  QTest::newRow("pitched note")
      << RowType::pitched_note_type << 0
      << static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 0
      << static_cast<int>(UnpitchedNoteColumn::number_of_unpitched_note_columns);
  QTest::newRow("pitched voice")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::number_of_pitched_voice_columns);
  QTest::newRow("unpitched voice")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::number_of_unpitched_voice_columns);
}

void Tester::test_column_count() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, number_of_columns);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(get_model(switch_table).columnCount(), number_of_columns);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}

void Tester::test_column_header_data() {
  add_table_columns();
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("column_name");

  QTest::newRow("chord interval")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_interval_column)
      << "Interval";
  QTest::newRow("chord beats")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_beats_column) << "Beats";
  QTest::newRow("chord velocity ratio")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("chord tempo ratio")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_tempo_ratio_column)
      << "Tempo ratio";
  QTest::newRow("chord words")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_words_column) << "Words";
  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_pitched_notes_column)
      << "Pitched notes";
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_unpitched_notes_column)
      << "Unpitched notes";
  QTest::newRow("pitched note voice")
      << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column)
      << "Voice";
  QTest::newRow("pitched note interval")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_interval_column) << "Interval";
  QTest::newRow("pitched note beats")
      << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_beats_column)
      << "Beats";
  QTest::newRow("pitched note velocity ratio")
      << RowType::pitched_note_type << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("pitched note words")
      << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_words_column)
      << "Words";
  QTest::newRow("unpitched note voice")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column)
      << "Voice";
  QTest::newRow("unpitched note beats")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column) << "Beats";
  QTest::newRow("unpitched note velocity ratio")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("unpitched note words")
      << RowType::unpitched_note_type << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column) << "Words";
  QTest::newRow("pitched voice name")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column) << "Name";
  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column) << "Instrument";
  QTest::newRow("pitched voice velocity ratio")
      << RowType::pitched_voice_type << -1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_velocity_ratio_column)
      << "Velocity ratio";
  QTest::newRow("unpitched voice name")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column) << "Name";
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column)
      << "Percussion set";
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column)
      << "MIDI number";
  QTest::newRow("unpitched voice velocity ratio")
      << RowType::unpitched_voice_type << -1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column)
      << "Velocity ratio";
}

void Tester::test_column_header() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, column_number);
  QFETCH(const QString, column_name);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(get_model(switch_table).headerData(column_number, Qt::Horizontal),
           column_name);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
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

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &edit_menu = song_editor.song_menu_bar.edit_menu;
  auto &undo_stack = song_widget.undo_stack;

  switch_to(song_editor, row_type, chord_number);

  auto &model = get_model(switch_table);
  auto &selector = get_selection_model(switch_table);

  const auto &first_index = model.index(first_row_number, column_number);
  const auto &second_index = model.index(second_row_number, column_number);

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

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &edit_menu = song_editor.song_menu_bar.edit_menu;
  auto &undo_stack = song_widget.undo_stack;

  switch_to(song_editor, row_type, chord_number);

  auto &model = get_model(switch_table);
  auto &selector = get_selection_model(switch_table);

  const auto &first_index = model.index(first_row_number, column_number);
  const auto &second_index = model.index(second_row_number, column_number);

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
  QTest::newRow("pitched voice name")
      << RowType::pitched_voice_type << -1 << 0
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column);
  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type << -1 << 0
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("unpitched voice name")
      << RowType::unpitched_voice_type << -1 << 0
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column);
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type << -1 << 0
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type << -1 << 0
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
}

void Tester::test_delete() {
  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &undo_stack = song_widget.undo_stack;

  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  switch_to(song_editor, row_type, chord_number);

  const auto delete_index =
      get_model(switch_table).index(row_number, column_number);
  const auto &old_value = delete_index.data();

  get_selection_model(switch_table).select(delete_index, SELECT_AND_CLEAR);
  song_editor.song_menu_bar.edit_menu.delete_cells_action.trigger();

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

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &view_menu = song_editor.song_menu_bar.view_menu;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(get_parent_chord_number(switch_table), chord_number);
  view_menu.previous_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_table), chord_number - 1);
  view_menu.next_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_table), chord_number);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
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
void Tester::test_reconnecting_selection_model_does_not_duplicate_connections() {
  auto &switch_table = song_editor.song_widget.switch_column.switch_table;
  auto &selection_model = get_selection_model(switch_table);

  // control: connecting without disconnecting first (the pre-fix pattern)
  // really does accumulate one call per reconnect
  {
    select_cell(switch_table, 0, 1);
    QObject receiver;
    auto call_count = 0;
    for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
      QObject::connect(&selection_model,
                       &QItemSelectionModel::selectionChanged, &receiver,
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
    QObject receiver;
    auto call_count = 0;
    for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
      QObject::disconnect(&selection_model,
                          &QItemSelectionModel::selectionChanged, &receiver,
                          nullptr);
      QObject::connect(&selection_model,
                       &QItemSelectionModel::selectionChanged, &receiver,
                       [&call_count]() -> auto { call_count = call_count + 1; });
    }
    select_cell(switch_table, 0, 0);
    QCOMPARE(call_count, 1);
  }
}

void Tester::test_replace_table_combining() {
  auto &song_widget = song_editor.song_widget;
  switch_to(song_editor, RowType::unpitched_note_type, 0);
  song_editor.song_menu_bar.view_menu.back_to_chords_action.trigger();
  QVERIFY(!song_widget.undo_stack.canUndo());
}

void Tester::test_row_count_data() {
  add_table_columns();
  QTest::addColumn<int>("number_of_rows");

  QTest::newRow("chords") << RowType::chord_type << -1 << EIGHT;
  QTest::newRow("chord 0 pitched notes") << RowType::pitched_note_type << 0 << 0;
  QTest::newRow("chord 1 pitched notes") << RowType::pitched_note_type << 1 << EIGHT;
  QTest::newRow("chord 2 pitched notes") << RowType::pitched_note_type << 2 << 0;
  QTest::newRow("chord 3 pitched notes") << RowType::pitched_note_type << 3 << 0;
  QTest::newRow("chord 4 pitched notes") << RowType::pitched_note_type << 4 << 0;
  QTest::newRow("chord 5 pitched notes") << RowType::pitched_note_type << FIVE << 0;
  QTest::newRow("chord 6 pitched notes") << RowType::pitched_note_type << SIX << 0;
  QTest::newRow("chord 7 pitched notes") << RowType::pitched_note_type << SEVEN << 0;
  QTest::newRow("chord 0 unpitched notes") << RowType::unpitched_note_type << 0 << 0;
  QTest::newRow("chord 1 unpitched notes") << RowType::unpitched_note_type << 1 << 4;
  QTest::newRow("chord 2 unpitched notes") << RowType::unpitched_note_type << 2 << 0;
  QTest::newRow("chord 3 unpitched notes") << RowType::unpitched_note_type << 3 << 0;
  QTest::newRow("chord 4 unpitched notes") << RowType::unpitched_note_type << 4 << 0;
  QTest::newRow("chord 5 unpitched notes")
      << RowType::unpitched_note_type << FIVE << 0;
  QTest::newRow("chord 6 unpitched notes") << RowType::unpitched_note_type << SIX << 0;
  QTest::newRow("chord 7 unpitched notes")
      << RowType::unpitched_note_type << SEVEN << 0;
  QTest::newRow("pitched voices") << RowType::pitched_voice_type << -1 << 2;
  QTest::newRow("unpitched voices") << RowType::unpitched_voice_type << -1 << 3;
}

void Tester::test_row_count() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, number_of_rows);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(get_model(switch_table).rowCount(), number_of_rows);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
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

  QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
               .headerData(0, Qt::Vertical, role),
           data);
}

void Tester::test_starting_control_data() {
  QTest::addColumn<QDoubleSpinBox *>("spin_box_pointer");
  QTest::addColumn<double *>("value_pointer");
  QTest::addColumn<double>("first_value");
  QTest::addColumn<double>("second_value");

  auto &song_widget = song_editor.song_widget;
  auto &song = song_widget.song;
  auto &spin_boxes = song_widget.controls_column.spin_boxes;

  QTest::newRow("key") << &spin_boxes.starting_key_editor
                       << &song.starting_key << STARTING_KEY_1
                       << STARTING_KEY_2;
  QTest::newRow("velocity")
      << &spin_boxes.starting_velocity_editor << &song.starting_velocity
      << STARTING_VELOCITY_1 << STARTING_VELOCITY_2;
  QTest::newRow("tempo") << &spin_boxes.starting_tempo_editor
                         << &song.starting_tempo << STARTING_TEMPO_1
                         << STARTING_TEMPO_2;
}

void Tester::test_starting_control() {
  QFETCH(QDoubleSpinBox *, spin_box_pointer);
  QFETCH(double *, value_pointer);
  QFETCH(const double, first_value);
  QFETCH(const double, second_value);

  auto &spin_box = get_reference(spin_box_pointer);
  auto &value = get_reference(value_pointer);

  const auto old_value = value;
  QCOMPARE_NE(old_value, first_value);
  QCOMPARE_NE(old_value, second_value);

  // test combining
  spin_box.setValue(first_value);
  QCOMPARE(value, first_value);
  spin_box.setValue(second_value);
  QCOMPARE(value, second_value);
  song_editor.song_widget.undo_stack.undo();
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
}

void Tester::test_status() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const QString, status);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole),
           status);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}

void Tester::test_set_value_data() {
  add_editable_cell_pairs();
  add_voice_column_pairs();
}

void Tester::test_set_value() {
  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &undo_stack = song_widget.undo_stack;

  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, first_row_number);
  QFETCH(const int, second_row_number);
  QFETCH(const int, column_number);

  switch_to(song_editor, row_type, chord_number);

  auto &model = get_model(switch_table);
  const auto second_index = model.index(second_row_number, column_number);

  // use EditRole rather than the default DisplayRole: for the voice
  // columns, DisplayRole shows the voice's name while EditRole (what the
  // cell editor actually reads/writes) is the underlying voice number, so
  // comparing/round-tripping DisplayRole values would bypass the editor
  const auto first_value =
      model.index(first_row_number, column_number).data(Qt::EditRole);
  const auto second_value = second_index.data(Qt::EditRole);
  QCOMPARE_NE(first_value, second_value);

  auto &delegate = get_reference(switch_table.itemDelegate());
  auto &cell_editor = get_reference(
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
      << SEVEN << static_cast<int>(ChordColumn::chord_interval_column) << "3/5o1";
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

  QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
               .index(row_number, column_number)
               .data()
               .toString(),
           text);
}

void Tester::test_unused_role_data() { add_tables(); }

void Tester::test_unused_role() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  auto &model = get_model(switch_table);
  const auto test_index = model.index(0, 0);
  QCOMPARE(test_index.data(Qt::DecorationRole), QVariant());
  QVERIFY(!(model.setData(test_index, QVariant(), Qt::DecorationRole)));
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}
