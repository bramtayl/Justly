#include "Tester.hpp"

void Tester::test_insert_after_data() { add_tables(); }

void Tester::test_insert_after() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);

  const auto old_row_count = model.rowCount();

  select_cell(switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();

  QCOMPARE(model.rowCount(), old_row_count + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), old_row_count);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_insert_into_data() { add_tables(); }

void Tester::test_insert_into() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);

  const auto old_row_count = model.rowCount();
  select_cell(switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_into_start_action
      .trigger();

  QCOMPARE(model.rowCount(), old_row_count + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), old_row_count);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_paste_after_data() { add_cells(); }

void Tester::test_paste_after() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);
  select_cell(switch_table, row_number, column_number);
  edit_menu.copy_action.trigger();

  const auto number_of_rows = model.rowCount();
  edit_menu.paste_menu.paste_after_action.trigger();
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), number_of_rows);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_paste_error_data() {
  add_table_columns();
  QTest::addColumn<QString>("copied");
  QTest::addColumn<QString>("mime_type");
  QTest::addColumn<QString>("error_message");
  QTest::newRow("chord empty clipboard")
      << RowType::chord_type << -1 << "" << "" << "Nothing to paste!";
  QTest::newRow("pitched note empty clipboard")
      << RowType::pitched_note_type << 1 << "" << "" << "Nothing to paste!";
  QTest::newRow("unpitched note empty clipboard")
      << RowType::unpitched_note_type << 1 << "" << "" << "Nothing to paste!";
  QTest::newRow("chord not a mime")
      << RowType::chord_type << -1 << "" << "not a mime"
      << "Cannot paste not a mime as chords cells";
  QTest::newRow("chord pitched notes mime")
      << RowType::chord_type << -1 << "" << PitchedNote::get_cells_mime()
      << "Cannot paste pitched notes cells as chords cells";
  QTest::newRow("chord not xml") << RowType::chord_type << -1 << "["
                                 << Chord::get_cells_mime() << "Invalid XML";
  QTest::newRow("chord not Justly")
      << RowType::chord_type << -1 << "<song/>" << Chord::get_cells_mime()
      << "Invalid clipboard";
  QTest::newRow("pitched note not a mime")
      << RowType::pitched_note_type << 1 << "" << "not a mime"
      << "Cannot paste not a mime as pitched notes cells";
  QTest::newRow("pitched note chords mime")
      << RowType::pitched_note_type << 1 << "" << Chord::get_cells_mime()
      << "Cannot paste chords cells as pitched notes cells";
  QTest::newRow("pitched note not xml")
      << RowType::pitched_note_type << 1 << "<" << PitchedNote::get_cells_mime()
      << "Invalid XML";
  QTest::newRow("pitched note not Justly")
      << RowType::pitched_note_type << 1 << "<song/>"
      << PitchedNote::get_cells_mime() << "Invalid clipboard";
  QTest::newRow("unpitched note not a mime")
      << RowType::unpitched_note_type << 1 << "" << "not a mime"
      << "Cannot paste not a mime as unpitched notes cells";
  QTest::newRow("unpitched note chords mime")
      << RowType::unpitched_note_type << 1 << "" << Chord::get_cells_mime()
      << "Cannot paste chords cells as unpitched notes cells";
  QTest::newRow("unpitched note not xml")
      << RowType::unpitched_note_type << 1 << "<"
      << UnpitchedNote::get_cells_mime() << "Invalid XML";
  QTest::newRow("unpitched note not Justly")
      << RowType::unpitched_note_type << 1 << "<song/>"
      << UnpitchedNote::get_cells_mime() << "Invalid clipboard";
}

void Tester::test_paste_error() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QString, error_message);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& new_data =
      get_reference(new QMimeData);  // NOLINT(cppcoreguidelines-owning-memory)

  // an empty mime type leaves the mime data with no formats at all
  if (!mime_type.isEmpty()) {
    new_data.setData(mime_type, copied.toStdString().c_str());
  }

  auto& clipboard = get_reference(QGuiApplication::clipboard());
  clipboard.setMimeData(&new_data);

  select_cell(switch_table, 0, 0);
  close_message_later(main_window, waiting_for_message, error_message);
  main_window.song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();

  maybe_switch_back_to_chords(undo_stack, row_type);
}

// inserting pasted rows rejects the same clipboards that pasting over does,
// without inserting anything
void Tester::test_paste_after_error_data() { test_paste_error_data(); }

void Tester::test_paste_after_error() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QString, error_message);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);
  const auto old_row_count = get_model(switch_table).rowCount(QModelIndex());

  auto& new_data =
      get_reference(new QMimeData);  // NOLINT(cppcoreguidelines-owning-memory)
  if (!mime_type.isEmpty()) {
    new_data.setData(mime_type, copied.toStdString().c_str());
  }
  get_clipboard().setMimeData(&new_data);

  select_cell(switch_table, 0, 0);
  close_message_later(main_window, waiting_for_message, error_message);
  main_window.song_menu_bar.edit_menu.paste_menu.paste_after_action.trigger();
  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), old_row_count);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_paste_into_data() { add_cells(); }

void Tester::test_paste_into() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;
  auto& edit_menu = main_window.song_menu_bar.edit_menu;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);

  select_cell(switch_table, row_number, column_number);
  edit_menu.copy_action.trigger();

  const auto number_of_rows = model.rowCount();
  edit_menu.paste_menu.paste_into_start_action.trigger();
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), number_of_rows);

  maybe_switch_back_to_chords(undo_stack, row_type);
}

void Tester::test_remove_row_data() { add_tables(); }

void Tester::test_remove_row() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);

  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;
  auto& undo_stack = window_body.undo_stack;

  switch_to(main_window, row_type, chord_number);

  auto& model = get_model(switch_table);

  const auto old_row_count = model.rowCount();

  select_cell(switch_table, 0, 0);
  // removing voice 0 reassigns the fixture's notes that use it, warning
  // about how many were reassigned
  if (row_type == RowType::pitched_voice_type) {
    close_message_later(
        main_window, waiting_for_message,
        "Reassigning 7 pitched note voices to the first voice \"Guitar\"");
  } else if (row_type == RowType::unpitched_voice_type) {
    close_message_later(
        main_window, waiting_for_message,
        "Reassigning 2 unpitched note voices to the first voice \"Room "
        "Kit\"");
  }
  main_window.song_menu_bar.edit_menu.remove_rows_action.trigger();

  QCOMPARE(model.rowCount(), old_row_count - 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), old_row_count);

  // undoing a row removal should select the restored row across every
  // column, not leave the selection empty
  const auto& restored_range = get_only_range(switch_table);
  QCOMPARE(restored_range.top(), 0);
  QCOMPARE(restored_range.bottom(), 0);
  QCOMPARE(restored_range.left(), 0);
  QCOMPARE(restored_range.right(), model.columnCount() - 1);

  maybe_switch_back_to_chords(undo_stack, row_type);
}
