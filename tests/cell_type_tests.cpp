#include <QDoubleSpinBox>

#include "Tester.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SpinBoxes.hpp"

void Tester::test_flag_data() {
  add_table_columns();
  QTest::addColumn<int>("column_number");
  QTest::addColumn<bool>("is_editable");

  QTest::newRow("chord interval")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_interval_column) << true;
  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_pitched_notes_column) << false;
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1
      << static_cast<int>(ChordColumn::chord_unpitched_notes_column) << false;
  QTest::newRow("pitched note") << RowType::pitched_note_type << 1 << 0 << true;
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 1 << 0 << true;
  QTest::newRow("pitched voice")
      << RowType::pitched_voice_type << -1 << 0 << true;
  QTest::newRow("unpitched voice")
      << RowType::unpitched_voice_type << -1 << 0 << true;
}

void Tester::test_flag() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, column_number);
  QFETCH(const bool, is_editable);

  const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;

  switch_to(song_editor, row_type, chord_number);
  QCOMPARE(
      get_model(switch_table).index(0, column_number).flags(),
      is_editable ? uneditable_flags | Qt::ItemIsEditable : uneditable_flags);
  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}

void Tester::test_frequency_bound_data() {
  QTest::addColumn<QPushButton*>("button_pointer");
  QTest::addColumn<QString>("error_message");

  auto& octave_row = song_editor.song_widget.controls_column.octave_row;

  QTest::newRow("too high")
      << &octave_row.plus_button
      << "Frequency 3.38e+05 for chord 2, pitched note 1 greater than or equal "
         "to maximum frequency 1.29e+04";
  QTest::newRow("too_low") << &octave_row.minus_button
                           << "Frequency 1.29 for chord 2, pitched note 1 "
                              "less than minimum frequency 7.94";
}

void Tester::test_frequency_bound() {
  QFETCH(QPushButton*, button_pointer);
  QFETCH(const QString, error_message);
  auto& button = get_reference(button_pointer);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, RowType::pitched_note_type, 1);
  select_cell(switch_table, 0, 0);
  close_message_later(song_editor, waiting_for_message, error_message);
  press_times(button, OCTAVE_SHIFT_TIMES);
  song_editor.song_menu_bar.play_menu.play_action.trigger();
  undo_times(song_widget.undo_stack, OCTAVE_SHIFT_TIMES);  // undo shift octave
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

void Tester::test_frequency_in_status_data() {
  QTest::addColumn<int>("frequency");
  QTest::addColumn<QString>("text");

  QTest::newRow("just below A")
      << A_MINUS_FREQUENCY
      << "217 Hz ≈ A3 − 24 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
  QTest::newRow("just above A")
      << A_PLUS_FREQUENCY
      << "223 Hz ≈ A3 + 23 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
  QTest::newRow("A")
      << A_FREQUENCY
      << "220 Hz ≈ A3; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("B♭")
      << B_FLAT_FREQUENCY
      << "233 Hz ≈ B♭3 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
  QTest::newRow("B")
      << B_FREQUENCY
      << "247 Hz ≈ B3; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("C") << C_FREQUENCY
                     << "262 Hz ≈ C4 + 2 cents; Velocity 10; 100 "
                        "bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("C♯")
      << C_SHARP_FREQUENCY
      << "277 Hz ≈ C♯4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
  QTest::newRow("D") << D_FREQUENCY
                     << "294 Hz ≈ D4 + 2 cents; Velocity 10; 100 "
                        "bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("E♭")
      << E_FLAT_FREQUENCY
      << "311 Hz ≈ E♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
  QTest::newRow("E") << E_FREQUENCY
                     << "330 Hz ≈ E4 + 2 cents; Velocity 10; 100 "
                        "bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("F") << F_FREQUENCY
                     << "349 Hz ≈ F4 − 1 cents; Velocity 10; 100 "
                        "bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("F♯")
      << F_SHARP_FREQUENCY
      << "370 Hz ≈ F♯4; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("G")
      << G_FREQUENCY
      << "392 Hz ≈ G4; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
  QTest::newRow("A♭")
      << A_FLAT_FREQUENCY
      << "415 Hz ≈ A♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
         "Duration 600 ms";
}

void Tester::test_frequency_in_status() {
  QFETCH(const int, frequency);
  QFETCH(const QString, text);

  auto& song_widget = song_editor.song_widget;

  song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
      frequency);
  QCOMPARE(get_model(song_widget.switch_column.switch_table)
               .index(0, 0)
               .data(Qt::StatusTipRole),
           text);
  song_widget.undo_stack.undo();
}

void Tester::test_gain() {
  auto& song_widget = song_editor.song_widget;
  auto& gain_editor = song_widget.controls_column.spin_boxes.gain_editor;

  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  gain_editor.setValue(NEW_GAIN_1);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
  gain_editor.setValue(NEW_GAIN_2);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_2);

  song_widget.undo_stack.undo();
  QCOMPARE(get_gain(song_widget), old_gain);
}

void Tester::test_interval_button_data() {
  auto& controls_column = song_editor.song_widget.controls_column;
  auto& third_row = controls_column.third_row;
  auto& fifth_row = controls_column.fifth_row;
  auto& seventh_row = controls_column.seventh_row;
  auto& octave_row = controls_column.octave_row;
  auto& third_minus_button = third_row.minus_button;
  auto& third_plus_button = third_row.plus_button;
  auto& fifth_minus_button = fifth_row.minus_button;
  auto& fifth_plus_button = fifth_row.plus_button;
  auto& seventh_minus_button = seventh_row.minus_button;
  auto& seventh_plus_button = seventh_row.plus_button;
  auto& octave_minus_button = octave_row.minus_button;
  auto& octave_plus_button = octave_row.plus_button;

  QTest::addColumn<QPushButton*>("button_pointer");
  add_table_columns();

  QTest::newRow("chord third -")
      << &third_minus_button << RowType::chord_type << -1;
  QTest::newRow("chord third +")
      << &third_plus_button << RowType::chord_type << -1;
  QTest::newRow("chord fifth -")
      << &fifth_minus_button << RowType::chord_type << -1;
  QTest::newRow("chord fifth +")
      << &fifth_plus_button << RowType::chord_type << -1;
  QTest::newRow("chord seventh -")
      << &seventh_minus_button << RowType::chord_type << -1;
  QTest::newRow("chord seventh +")
      << &seventh_plus_button << RowType::chord_type << -1;
  QTest::newRow("chord octave -")
      << &octave_minus_button << RowType::chord_type << -1;
  QTest::newRow("chord octave +")
      << &octave_plus_button << RowType::chord_type << -1;
  QTest::newRow("pitched note third -")
      << &third_minus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note third +")
      << &third_plus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note fifth -")
      << &fifth_minus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note fifth +")
      << &fifth_plus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note seventh -")
      << &seventh_minus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note seventh +")
      << &seventh_plus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note octave -")
      << &octave_minus_button << RowType::pitched_note_type << 1;
  QTest::newRow("pitched note octave +")
      << &octave_plus_button << RowType::pitched_note_type << 1;
}

void Tester::test_interval_button() {
  QFETCH(QPushButton*, button_pointer);
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, row_type, chord_number);
  const auto test_index =
      get_model(switch_table)
          .index(0, row_type == RowType::chord_type
                        ? static_cast<int>(ChordColumn::chord_interval_column)
                        : static_cast<int>(
                              PitchedNoteColumn::pitched_note_interval_column));
  const auto original_data = test_index.data();
  get_selection_model(switch_table).select(test_index, SELECT_AND_CLEAR);

  get_reference(button_pointer).click();
  QCOMPARE_NE(original_data, test_index.data());
  undo_stack.undo();
  QCOMPARE(original_data, test_index.data());
  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_unreduced_ratio_from_xml_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<int>("column_number");
  QTest::addColumn<QString>("expected_text");

  static const QString header =
      "<song><gain>1</gain><starting_key>220</starting_key>"
      "<starting_tempo>100</starting_tempo><starting_velocity>10</"
      "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
      "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
      "<unpitched_voices><unpitched_voice><name>B</name>"
      "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
      "midi_number></unpitched_voice></unpitched_voices><chords><chord>";
  static const QString footer = "</chord></chords></song>";

  QTest::newRow("unreduced ratio folds to default")
      << header +
             "<velocity_ratio><numerator>2</numerator><denominator>2</"
             "denominator></velocity_ratio>" +
             footer
      << static_cast<int>(ChordColumn::chord_velocity_ratio_column) << "";
  QTest::newRow("unreduced ratio folds by gcd")
      << header +
             "<velocity_ratio><numerator>4</numerator><denominator>6</"
             "denominator></velocity_ratio>" +
             footer
      << static_cast<int>(ChordColumn::chord_velocity_ratio_column) << "2/3";
  QTest::newRow("unreduced interval ratio folds into octave")
      << header +
             "<interval><ratio><numerator>4</numerator></ratio></interval>" +
             footer
      << static_cast<int>(ChordColumn::chord_interval_column) << "o2";
}

// regression test: a chord's ratio/interval loaded from XML whose
// numerator/denominator aren't already coprime (the schema only bounds
// each to [1, 999], not that the pair is reduced) must be normalized to
// the same canonical form every Rational/Interval constructor produces.
// Otherwise the cell displays a value the UI can never actually produce
// (e.g. "2/2" instead of blank), and re-committing the unchanged value
// through the editor -- which always writes back a reduced value -- looks
// like a real edit and pushes a spurious undo entry.
void Tester::test_unreduced_ratio_from_xml() {
  QFETCH(const QString, text);
  QFETCH(const int, column_number);
  QFETCH(const QString, expected_text);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  open_text(song_editor, text);

  auto& model = get_model(switch_table);
  const auto test_index = model.index(0, column_number);

  QCOMPARE(test_index.data().toString(), expected_text);

  const auto old_undo_count = undo_stack.count();
  auto& delegate = get_reference(switch_table.itemDelegate());
  auto& cell_editor = get_reference(
      delegate.createEditor(&get_reference(switch_table.viewport()),
                            QStyleOptionViewItem(), test_index));
  delegate.setEditorData(&cell_editor, test_index);
  delegate.setModelData(&cell_editor, &model, test_index);

  QCOMPARE(undo_stack.count(), old_undo_count);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// regression test: Interval's folding constructor used to halve the
// numerator while it was even without checking for 0 first -- 0 % 2 == 0
// forever, so a 0 numerator hung the constructor instead of terminating.
// Not reachable through the UI or schema-validated XML today (both bound
// numerator to >= 1), but the constructor shouldn't hang on it regardless.
void Tester::test_interval_zero_numerator_does_not_hang() {
  Rational zero_ratio;
  zero_ratio.numerator = 0;
  zero_ratio.denominator = 1;

  const Interval interval(zero_ratio, 3);

  QCOMPARE(interval.ratio.numerator, 0);
  QCOMPARE(interval.ratio.denominator, 1);
  QCOMPARE(interval.octave, 3);
}

// regression test: some musicxml fields (e.g. fifths, octave-change,
// divisions) are unbounded xs:integer/xs:decimal with no schema-enforced
// range, so std::stoi can throw std::out_of_range on a magnitude that
// doesn't fit in int; string_to_maybe_int must report that as nullopt
// instead of letting the exception propagate uncaught and crash the app
void Tester::test_string_to_maybe_int_data() {
  QTest::addColumn<QString>("content");
  QTest::addColumn<bool>("expect_value");

  QTest::newRow("ordinary value") << "42" << true;
  QTest::newRow("overflow positive") << "99999999999999999999" << false;
  QTest::newRow("overflow negative") << "-99999999999999999999" << false;
}

void Tester::test_string_to_maybe_int() {
  QFETCH(const QString, content);
  QFETCH(const bool, expect_value);

  QCOMPARE(string_to_maybe_int(content.toStdString()).has_value(),
           expect_value);
}

void Tester::test_octave_bound() {
  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, RowType::pitched_note_type, 1);
  select_cell(switch_table, 0, 0);
  close_message_later(song_editor, waiting_for_message,
                      "Octave 10 (absolutely) greater than maximum 9");
  press_times(song_widget.controls_column.octave_row.plus_button,
              OCTAVE_SHIFT_TIMES + 1);
  undo_times(undo_stack, OCTAVE_SHIFT_TIMES);  // undo shift octave

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

void Tester::test_ratio_bound_data() {
  QTest::addColumn<QPushButton*>("fifth_button_pointer");
  QTest::addColumn<QPushButton*>("octave_button_pointer");
  QTest::addColumn<QString>("error_message");

  auto& controls_column = song_editor.song_widget.controls_column;
  auto& fifth_row = controls_column.fifth_row;
  auto& octave_row = controls_column.octave_row;

  QTest::newRow("too high") << &fifth_row.plus_button << &octave_row.plus_button
                            << "Numerator 2187 greater than maximum 999";

  QTest::newRow("too low") << &fifth_row.minus_button
                           << &octave_row.minus_button
                           << "Denominator 2187 greater than maximum 999";
}

void Tester::test_ratio_bound() {
  QFETCH(QPushButton*, fifth_button_pointer);
  QFETCH(QPushButton*, octave_button_pointer);
  QFETCH(const QString, error_message);

  auto& fifth_button = get_reference(fifth_button_pointer);
  auto& octave_button = get_reference(octave_button_pointer);

  auto& song_widget = song_editor.song_widget;
  auto& switch_table = song_widget.switch_column.switch_table;
  auto& undo_stack = song_widget.undo_stack;

  switch_to(song_editor, RowType::pitched_note_type, 1);
  select_cell(switch_table, 0, 0);

  for (auto counter = 0; counter < RATIO_SHIFT_TIMES; counter++) {
    fifth_button.click();
    octave_button.click();
  }
  close_message_later(song_editor, waiting_for_message, error_message);
  fifth_button.click();
  undo_times(undo_stack, RATIO_SHIFT_TIMES * 2);  // undo shift numerator

  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}
