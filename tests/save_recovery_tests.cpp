#include <QDoubleSpinBox>
#include <QtCore/QSettings>
#include <QtGui/QCloseEvent>
#include <QtTest/QSignalSpy>

#include "Tester.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"

void Tester::test_save() {
  auto& window_body = main_window.window_body;
  auto& song_menu_bar = main_window.song_menu_bar;

  auto original_text = get_file_text(test_dir.filePath("test_song.xml"));

  // save into a temp file, driven through the actual Save As dialog
  // (rather than calling save_as_file directly) so FileMenu's
  // dialog-accept wiring -- maybe_choose_file and the
  // save_as_action lambda itself -- gets exercised too
  auto save_filename = test_dir.filePath("test_song_2.xml");
  // must not already exist -- accept() would otherwise pop up an
  // overwrite-confirmation box nobody is waiting to close
  QFile::remove(save_filename);
  accept_file_dialog_later(main_window, save_filename);
  song_menu_bar.file_menu.save_as_action.trigger();
  QCOMPARE(window_body.current_file, save_filename);

  // compare the saved text to the original text
  QCOMPARE(original_text, get_file_text(save_filename));

  // now change the song and save to the same file
  window_body.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  song_menu_bar.file_menu.save_action.trigger();
  window_body.undo_stack.undo();

  QCOMPARE_NE(original_text, get_file_text(save_filename));

  QFile(save_filename).remove();
}

void Tester::test_save_error_does_not_lose_work() {
  auto& window_body = main_window.window_body;
  auto& undo_stack = window_body.undo_stack;

  const auto old_current_file = window_body.current_file;

  write_recovery_file(window_body);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  window_body.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  QVERIFY(!undo_stack.isClean());

  // a directory can never be opened for writing as a file, so this
  // deterministically fails xmlSaveFile without depending on filesystem
  // permissions
  const auto unwritable_path = test_dir.filePath("test_save_error_dir");
  QDir(unwritable_path).removeRecursively();
  QVERIFY(QDir().mkpath(unwritable_path));

  close_message_later(main_window, waiting_for_message, "Failed to save file");
  save_as_file(window_body, unwritable_path);

  // a failed save must not be mistaken for a successful one: the current
  // file, dirty undo stack, and recovery file are all still what they were
  // before the failed save attempt
  QCOMPARE(window_body.current_file, old_current_file);
  QVERIFY(!undo_stack.isClean());
  QVERIFY(QFile::exists(get_recovery_file_path()));

  QDir(unwritable_path).removeRecursively();
  undo_stack.undo();
  remove_recovery_file();
}

void Tester::test_recovery_removed_on_save_and_open() {
  auto& window_body = main_window.window_body;
  auto fixture_file = test_dir.filePath("test_song.xml");

  write_recovery_file(window_body);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  auto save_filename = test_dir.filePath("test_recovery_save.xml");
  save_as_file(window_body, save_filename);
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QFile(save_filename).remove();

  write_recovery_file(window_body);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  // reloading also restores current_file/song state for later tests
  open_file(main_window, fixture_file);
  QVERIFY(!QFile::exists(get_recovery_file_path()));
}

void Tester::test_recovery_timer_debounce() {
  auto& window_body = main_window.window_body;
  auto& gain_editor = window_body.controls_column.spin_boxes.gain_editor;
  auto& recovery_timer = window_body.recovery_timer;

  remove_recovery_file();
  // other tests' edits may still have the debounce timer counting down
  // from earlier in the run -- start from a known-stopped state instead of
  // asserting on that incidental timing
  recovery_timer.stop();

  const auto old_gain = get_gain(window_body.player);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  gain_editor.setValue(NEW_GAIN_1);
  QVERIFY(recovery_timer.isActive());

  // force the debounce timer to fire now rather than waiting out the real
  // multi-second interval
  QSignalSpy timeout_spy(&recovery_timer, &QTimer::timeout);
  recovery_timer.start(0);
  QVERIFY(timeout_spy.wait());
  QVERIFY(QFile::exists(get_recovery_file_path()));

  window_body.undo_stack.undo();
  QCOMPARE(get_gain(window_body.player), old_gain);
  QVERIFY(recovery_timer.isActive());

  recovery_timer.start(0);
  QVERIFY(timeout_spy.wait());
  // back at the clean index, so the debounced write removes rather than
  // rewrites the now-stale recovery file
  QVERIFY(!QFile::exists(get_recovery_file_path()));
}

void Tester::test_recovery_restore_accepted() {
  auto& window_body = main_window.window_body;
  auto fixture_file = test_dir.filePath("test_song.xml");

  open_file(main_window, fixture_file);
  const auto old_gain = get_gain(window_body.player);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);

  window_body.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  write_recovery_file(window_body);
  // undoing approximates relaunching without the unsaved edit -- the
  // recovery file itself is untouched, since only save/open/import/close
  // clear it, not undo
  window_body.undo_stack.undo();
  QCOMPARE(get_gain(window_body.player), old_gain);

  answer_question_later(main_window, waiting_for_message, RECOVERY_PROMPT_TEXT,
                        QMessageBox::Yes);
  QVERIFY(maybe_restore_recovery(main_window));

  QCOMPARE(get_gain(window_body.player), NEW_GAIN_1);
  QCOMPARE(window_body.current_file, fixture_file);
  QVERIFY(!window_body.undo_stack.isClean());
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QVERIFY(!QSettings().contains("recovery/original_file"));

  open_file(main_window, fixture_file);
}

void Tester::test_recovery_restore_declined() {
  auto& window_body = main_window.window_body;
  auto fixture_file = test_dir.filePath("test_song.xml");

  open_file(main_window, fixture_file);
  const auto old_gain = get_gain(window_body.player);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);

  window_body.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  write_recovery_file(window_body);
  window_body.undo_stack.undo();

  answer_question_later(main_window, waiting_for_message, RECOVERY_PROMPT_TEXT,
                        QMessageBox::No);
  QVERIFY(!maybe_restore_recovery(main_window));

  QCOMPARE(get_gain(window_body.player), old_gain);
  QCOMPARE(window_body.current_file, fixture_file);
  QVERIFY(window_body.undo_stack.isClean());
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QVERIFY(!QSettings().contains("recovery/original_file"));
}

void Tester::test_recovery_no_prompt_when_missing() {
  remove_recovery_file();
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  // the class-wide unexpected_message_timer watchdog fails the test if a
  // dialog appears here
  QVERIFY(!maybe_restore_recovery(main_window));
}

// closing with unsaved changes asks first; declining must keep the window
// open (event ignored) and leave any recovery file in place
void Tester::test_close_event_discard_declined() {
  auto& window_body = main_window.window_body;
  auto& switch_table = window_body.switch_column.switch_table;

  select_cell(switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
  QVERIFY(!window_body.undo_stack.isClean());
  write_recovery_file(window_body);

  answer_question_later(main_window, waiting_for_message,
                        "Discard unsaved changes?", QMessageBox::No);
  QCloseEvent close_event;
  main_window.closeEvent(&close_event);

  QVERIFY(!close_event.isAccepted());
  QVERIFY(QFile::exists(get_recovery_file_path()));

  // restore the shared fixture (also removes the recovery file)
  open_file(main_window, test_dir.filePath("test_song.xml"));
}

// a clean shutdown must delete the crash-recovery file, since its presence
// is what tells the next launch that the last session crashed
void Tester::test_close_event_removes_recovery_file() {
  auto& window_body = main_window.window_body;

  QVERIFY(window_body.undo_stack.isClean());
  write_recovery_file(window_body);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  QCloseEvent close_event;
  main_window.closeEvent(&close_event);

  QVERIFY(close_event.isAccepted());
  QVERIFY(!QFile::exists(get_recovery_file_path()));
}

// a recovery write that fails leaves the previous recovery state alone,
// rather than recording an original file for content that was never written
void Tester::test_recovery_write_failure() {
  auto& window_body = main_window.window_body;

  remove_recovery_file();
  QVERIFY(!QSettings().contains("recovery/original_file"));

  // a directory where recovery.xml should go makes writing it fail
  const auto recovery_path = get_recovery_file_path();
  QVERIFY(QDir().mkpath(recovery_path));
  write_recovery_file(window_body);
  QVERIFY(!QSettings().contains("recovery/original_file"));

  QVERIFY(QDir(recovery_path).removeRecursively());
}

// accepting a recovery file that can't be opened reports the error and
// leaves the current song as it was
void Tester::test_recovery_restore_invalid_file() {
  auto& window_body = main_window.window_body;
  const auto old_current_file = window_body.current_file;

  QFile recovery_file(get_recovery_file_path());
  QVERIFY(recovery_file.open(QIODevice::WriteOnly));
  recovery_file.write("<");
  recovery_file.close();

  // Enter picks the prompt's default Yes button
  close_messages_later(main_window, waiting_for_message,
                       {RECOVERY_PROMPT_TEXT, "Invalid XML file"});
  QVERIFY(!maybe_restore_recovery(main_window));
  QVERIFY(!waiting_for_message);

  QCOMPARE(window_body.current_file, old_current_file);
  QVERIFY(window_body.undo_stack.isClean());
  remove_recovery_file();
}
