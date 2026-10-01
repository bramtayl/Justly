#ifdef __linux__
#include <sys/resource.h>

#include <csignal>
#endif

#include "Tester.hpp"

void Tester::test_export() {
  auto& window_body = main_window.window_body;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(window_body, temp_export_file.fileName());
}

// regression test: test_export calls export_to_file directly, which never
// exercises FileMenu's export_action lambda (make_file_dialog,
// get_selected_file, and the call to export_to_file itself) -- drive it
// through the actual dialog instead
void Tester::test_export_via_dialog() {
  auto& song_menu_bar = main_window.song_menu_bar;

  // a path that doesn't exist yet -- unlike QTemporaryFile, which
  // pre-creates the file and would make QFileDialog::accept() pop up an
  // "already exists" overwrite-confirmation box nobody is waiting to
  // close
  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto export_filename = temp_export_dir.filePath("export.wav");

  accept_file_dialog_later(main_window, export_filename);
  song_menu_bar.file_menu.export_action.trigger();

  QFile written_file(export_filename);
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QVERIFY(written_file.size() > 0);
}

// regression test: an export path whose parent directory doesn't exist
// makes new_fluid_file_renderer fail to open the file -- export_to_file
// should warn instead of dereferencing the null renderer pointer, and
// should still restore playback state (synth.lock-memory, audio driver)
// afterward rather than leaving the app stuck with realtime audio off
void Tester::test_export_unwritable_path() {
  auto& window_body = main_window.window_body;

  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto unwritable_path =
      temp_export_dir.filePath("nonexistent_subdir/export.wav");

  close_message_later(main_window, waiting_for_message, "Cannot write to file");
  export_to_file(window_body, unwritable_path);

  QVERIFY(!QFile::exists(unwritable_path));

  // a subsequent export to a valid path should still work, confirming
  // export_to_file restored playback state rather than leaving it broken
  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(window_body, temp_export_file.fileName());
}

// a file size limit lets the renderer open its output file and then fails
// its writes partway through the export, which a full disk would do too
void Tester::test_export_write_error() {
#ifdef __linux__
  static const rlim_t SMALL_FILE_SIZE_LIMIT = 4096;

  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  const auto export_filename = temp_export_dir.filePath("export.wav");

  rlimit old_limit{};
  QCOMPARE(getrlimit(RLIMIT_FSIZE, &old_limit), 0);
  auto* const old_handler = std::signal(SIGXFSZ, SIG_IGN);
  auto new_limit = old_limit;
  new_limit.rlim_cur = SMALL_FILE_SIZE_LIMIT;
  QCOMPARE(setrlimit(RLIMIT_FSIZE, &new_limit), 0);

  close_message_later(main_window, waiting_for_message, "Error writing file");
  export_to_file(main_window.window_body, export_filename);

  QCOMPARE(setrlimit(RLIMIT_FSIZE, &old_limit), 0);
  static_cast<void>(std::signal(SIGXFSZ, old_handler));
#else
  QSKIP("file size limits are Linux-specific");
#endif
}

// regression test: FileMenu's dialogs (make_file_dialog) must not leak --
// Open/Import/Save As/Export used to create a new QFileDialog
// with no matching deleteLater(), so every use of a file dialog left a
// live QFileDialog parented to window_body for the rest of the process
void Tester::test_file_dialog_cleanup() {
  auto& file_menu = main_window.song_menu_bar.file_menu;

  QPointer<QFileDialog> dialog_pointer;
  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&main_window));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &main_window,
                   [&dialog_pointer]() -> auto {
                     auto* const found_dialog = find_top_level_file_dialog();
                     QVERIFY(found_dialog != nullptr);
                     dialog_pointer = found_dialog;
                     found_dialog->reject();
                   });
  timer.start(WAIT_TIME);

  file_menu.save_as_action.trigger();

  // the dialog is still alive right after trigger() returns -- deleteLater()
  // only schedules its destruction for the next trip through the event loop
  QVERIFY(!dialog_pointer.isNull());
  QTRY_VERIFY(dialog_pointer.isNull());
}
