#include <QtCore/QSettings>
#include <QtTest/QSignalSpy>
#include "Tester.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/SpinBoxes.hpp"

void Tester::test_save() {
  auto& song_widget = song_editor.song_widget;
  auto& song_menu_bar = song_editor.song_menu_bar;

  auto original_text = get_file_text(test_dir.filePath("test_song.xml"));

  // save into a temp file, driven through the actual Save As dialog
  // (rather than calling save_as_file directly) so FileMenu's
  // dialog-accept wiring -- make_file_dialog, get_selected_file, and the
  // save_as_action lambda itself -- gets exercised too
  auto save_filename = test_dir.filePath("test_song_2.xml");
  // must not already exist -- accept() would otherwise pop up an
  // overwrite-confirmation box nobody is waiting to close
  QFile::remove(save_filename);
  accept_file_dialog_later(song_editor, save_filename);
  song_menu_bar.file_menu.save_as_action.trigger();
  QCOMPARE(song_widget.current_file, save_filename);

  // compare the saved text to the original text
  QCOMPARE(original_text, get_file_text(save_filename));

  // now change the song and save to the same file
  song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  song_menu_bar.file_menu.save_action.trigger();
  song_widget.undo_stack.undo();

  QCOMPARE_NE(original_text, get_file_text(save_filename));

  QFile(save_filename).remove();
}

void Tester::test_save_error_does_not_lose_work() {
  auto& song_widget = song_editor.song_widget;
  auto& undo_stack = song_widget.undo_stack;

  const auto old_current_file = song_widget.current_file;

  write_recovery_file(song_widget);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  QVERIFY(!undo_stack.isClean());

  // a directory can never be opened for writing as a file, so this
  // deterministically fails xmlSaveFile without depending on filesystem
  // permissions
  const auto unwritable_path = test_dir.filePath("test_save_error_dir");
  QDir(unwritable_path).removeRecursively();
  QVERIFY(QDir().mkpath(unwritable_path));

  close_message_later(song_editor, waiting_for_message, "Failed to save file");
  save_as_file(song_widget, unwritable_path);

  // a failed save must not be mistaken for a successful one: the current
  // file, dirty undo stack, and recovery file are all still what they were
  // before the failed save attempt
  QCOMPARE(song_widget.current_file, old_current_file);
  QVERIFY(!undo_stack.isClean());
  QVERIFY(QFile::exists(get_recovery_file_path()));

  QDir(unwritable_path).removeRecursively();
  undo_stack.undo();
  remove_recovery_file();
}

void Tester::test_recovery_removed_on_save_and_open() {
  auto& song_widget = song_editor.song_widget;
  auto fixture_file = test_dir.filePath("test_song.xml");

  write_recovery_file(song_widget);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  auto save_filename = test_dir.filePath("test_recovery_save.xml");
  save_as_file(song_widget, save_filename);
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QFile(save_filename).remove();

  write_recovery_file(song_widget);
  QVERIFY(QFile::exists(get_recovery_file_path()));

  // reloading also restores current_file/song state for later tests
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, fixture_file);
  QVERIFY(!QFile::exists(get_recovery_file_path()));
}

void Tester::test_recovery_timer_debounce() {
  auto& song_widget = song_editor.song_widget;
  auto& gain_editor = song_widget.controls_column.spin_boxes.gain_editor;
  auto& recovery_timer = song_widget.recovery_timer;

  remove_recovery_file();
  // other tests' edits may still have the debounce timer counting down
  // from earlier in the run -- start from a known-stopped state instead of
  // asserting on that incidental timing
  recovery_timer.stop();

  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  gain_editor.setValue(NEW_GAIN_1);
  QVERIFY(recovery_timer.isActive());

  // force the debounce timer to fire now rather than waiting out the real
  // multi-second interval
  QSignalSpy timeout_spy(&recovery_timer, &QTimer::timeout);
  recovery_timer.start(0);
  QVERIFY(timeout_spy.wait());
  QVERIFY(QFile::exists(get_recovery_file_path()));

  song_widget.undo_stack.undo();
  QCOMPARE(get_gain(song_widget), old_gain);
  QVERIFY(recovery_timer.isActive());

  recovery_timer.start(0);
  QVERIFY(timeout_spy.wait());
  // back at the clean index, so the debounced write removes rather than
  // rewrites the now-stale recovery file
  QVERIFY(!QFile::exists(get_recovery_file_path()));
}

void Tester::test_recovery_restore_accepted() {
  auto& song_widget = song_editor.song_widget;
  auto fixture_file = test_dir.filePath("test_song.xml");

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, fixture_file);
  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);

  song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  write_recovery_file(song_widget);
  // undoing approximates relaunching without the unsaved edit -- the
  // recovery file itself is untouched, since only save/open/import/close
  // clear it, not undo
  song_widget.undo_stack.undo();
  QCOMPARE(get_gain(song_widget), old_gain);

  answer_question_later(song_editor, waiting_for_message, RECOVERY_PROMPT_TEXT,
                        QMessageBox::Yes);
  QVERIFY(maybe_restore_recovery(song_widget));

  QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
  QCOMPARE(song_widget.current_file, fixture_file);
  QVERIFY(!song_widget.undo_stack.isClean());
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QVERIFY(!QSettings().contains("recovery/original_file"));

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, fixture_file);
}

void Tester::test_recovery_restore_declined() {
  auto& song_widget = song_editor.song_widget;
  auto fixture_file = test_dir.filePath("test_song.xml");

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, fixture_file);
  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);

  song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
  write_recovery_file(song_widget);
  song_widget.undo_stack.undo();

  answer_question_later(song_editor, waiting_for_message, RECOVERY_PROMPT_TEXT,
                        QMessageBox::No);
  QVERIFY(!maybe_restore_recovery(song_widget));

  QCOMPARE(get_gain(song_widget), old_gain);
  QCOMPARE(song_widget.current_file, fixture_file);
  QVERIFY(song_widget.undo_stack.isClean());
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  QVERIFY(!QSettings().contains("recovery/original_file"));
}

void Tester::test_recovery_no_prompt_when_missing() {
  remove_recovery_file();
  QVERIFY(!QFile::exists(get_recovery_file_path()));
  // the class-wide unexpected_message_timer watchdog fails the test if a
  // dialog appears here
  QVERIFY(!maybe_restore_recovery(song_editor.song_widget));
}
