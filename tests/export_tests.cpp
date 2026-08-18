#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtCore/Qt>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtTest/QTest>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLineEdit>
#include <qtestkeyboard.h>

#include "Tester.hpp"
#include "menus/FileMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "other/MidiTrackEvent.hpp"
#include "test_helpers.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"

void Tester::test_export() {
  auto &song_widget = song_editor.song_widget;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(song_widget, temp_export_file.fileName());
}

void Tester::test_export_midi() {
  auto &song_widget = song_editor.song_widget;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  // the fixture's chord 2 has three different unpitched voices starting
  // at the same tick, which can't all occupy GM's single shared
  // percussion channel at once -- export aborts on the first conflict
  // rather than silently writing a file missing notes the user didn't
  // ask to drop
  close_message_later(
      song_editor, waiting_for_message,
      "Percussion instrument Room for chord 2, unpitched note 2 starts "
      "at the same time as a different percussion instrument on the "
      "shared MIDI percussion channel");
  export_midi_to_file(song_widget, temp_export_file.fileName());

  QFile written_file(temp_export_file.fileName());
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QCOMPARE(written_file.read(4), QByteArray());
}

// regression test: test_export calls export_to_file directly, which never
// exercises FileMenu's export_action lambda (make_file_dialog,
// get_selected_file, and the call to export_to_file itself) -- drive it
// through the actual dialog instead
void Tester::test_export_via_dialog() {
  auto &song_menu_bar = song_editor.song_menu_bar;

  // a path that doesn't exist yet -- unlike QTemporaryFile, which
  // pre-creates the file and would make QFileDialog::accept() pop up an
  // "already exists" overwrite-confirmation box nobody is waiting to
  // close
  QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto export_filename = temp_export_dir.filePath("export.wav");

  accept_file_dialog_later(song_editor, export_filename);
  song_menu_bar.file_menu.export_action.trigger();

  QFile written_file(export_filename);
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QVERIFY(written_file.size() > 0);
}

// regression test: same gap as test_export_via_dialog, but for
// export_midi_action. Accepting the dialog triggers export_midi_to_file,
// which (like test_export_midi) hits the fixture's percussion-channel
// conflict and pops up a second, nested modal dialog -- so the file
// dialog is accepted after a short delay, and the resulting message box
// is closed by a separately-armed timer that fires later
void Tester::test_export_midi_via_dialog() {
  auto &song_menu_bar = song_editor.song_menu_bar;

  // see test_export_via_dialog: a path that doesn't exist yet, so
  // accept() doesn't pop up an overwrite-confirmation box on top of the
  // percussion-conflict warning this test already expects
  QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto export_filename = temp_export_dir.filePath("export.mid");

  close_message_later(
      song_editor, waiting_for_message,
      "Percussion instrument Room for chord 2, unpitched note 2 starts "
      "at the same time as a different percussion instrument on the "
      "shared MIDI percussion channel");

  auto &dialog_timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&song_editor));
  dialog_timer.setSingleShot(true);
  QObject::connect(&dialog_timer, &QTimer::timeout, &song_editor,
                   [export_filename]() -> auto {
                     auto *const found_dialog = find_top_level_file_dialog();
                     QVERIFY(found_dialog != nullptr);
                     // see accept_file_dialog_later: type into the
                     // filename line edit directly rather than using
                     // selectFile(), which only sets the directory for a
                     // not-yet-existing target
                     auto *const line_edit = found_dialog->findChild<QLineEdit *>();
                     QVERIFY(line_edit != nullptr);
                     line_edit->setText(export_filename);
                     QTest::keyEvent(QTest::Press, line_edit, Qt::Key_Enter);
                   });
  // fires well before close_message_later's own timer, so the dialog is
  // accepted (and the percussion-conflict warning shown) before that
  // timer goes looking for the message box
  dialog_timer.start(WAIT_TIME / 2);

  song_menu_bar.file_menu.export_midi_action.trigger();
}

// regression test: an export path whose parent directory doesn't exist
// makes new_fluid_file_renderer fail to open the file -- export_to_file
// should warn instead of dereferencing the null renderer pointer, and
// should still restore playback state (synth.lock-memory, audio driver)
// afterward rather than leaving the app stuck with realtime audio off
void Tester::test_export_unwritable_path() {
  auto &song_widget = song_editor.song_widget;

  QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto unwritable_path =
      temp_export_dir.filePath("nonexistent_subdir/export.wav");

  close_message_later(song_editor, waiting_for_message,
                      "Cannot write to file");
  export_to_file(song_widget, unwritable_path);

  QVERIFY(!QFile::exists(unwritable_path));

  // a subsequent export to a valid path should still work, confirming
  // export_to_file restored playback state rather than leaving it broken
  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(song_widget, temp_export_file.fileName());
}

// regression test: FileMenu's dialogs (make_file_dialog) must not leak --
// Open/Import/Save As/Export/Export MIDI used to create a new QFileDialog
// with no matching deleteLater(), so every use of a file dialog left a
// live QFileDialog parented to song_widget for the rest of the process
void Tester::test_file_dialog_cleanup() {
  auto &file_menu = song_editor.song_menu_bar.file_menu;

  QPointer<QFileDialog> dialog_pointer;
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&song_editor));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &song_editor,
                   [&dialog_pointer]() -> auto {
                     auto *const found_dialog = find_top_level_file_dialog();
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

// MidiTrackEvent.hpp's byte-encoding helpers are only reachable through
// export_midi_to_file's happy path, which every export test above avoids
// -- the fixture's chord 2 always hits the percussion-channel conflict
// and returns before any bytes are written. Exercise the encoding
// directly instead of depending on fixture content that isn't meant to
// support a conflict-free export.
void Tester::test_midi_append_variable_length_data() {
  QTest::addColumn<unsigned int>("value");
  QTest::addColumn<QByteArray>("expected_hex");

  // fits in a single septet, no continuation bit
  QTest::newRow("zero") << 0U << QByteArray("00");
  QTest::newRow("largest single byte") << 127U << QByteArray("7F");
  // smallest value needing a second, continuation-flagged byte
  QTest::newRow("smallest two byte") << 128U << QByteArray("8100");
  QTest::newRow("two byte") << 300U << QByteArray("822C");
  QTest::newRow("three byte") << 16384U << QByteArray("818000");
}

void Tester::test_midi_append_variable_length() {
  QFETCH(const unsigned int, value);
  QFETCH(const QByteArray, expected_hex);

  QByteArray bytes;
  append_variable_length(bytes, value);
  QCOMPARE(bytes, QByteArray::fromHex(expected_hex));
}

void Tester::test_midi_byte_encoding() {
  QByteArray bytes;

  append_meta_event(bytes, MIDI_TEMPO_META_TYPE, QByteArray::fromHex("07A120"));
  QCOMPARE(bytes, QByteArray::fromHex("FF510307A120"));

  bytes.clear();
  append_track_name_meta(bytes, "Hi");
  QCOMPARE(bytes, QByteArray::fromHex("FF0302") + QByteArray("Hi"));

  bytes.clear();
  append_control_change(bytes, 3, 7, 100);
  QCOMPARE(bytes, QByteArray::fromHex("B30764"));

  bytes.clear();
  append_program_change(bytes, 2, 5);
  QCOMPARE(bytes, QByteArray::fromHex("C205"));

  bytes.clear();
  append_note_on(bytes, 1, 60, 100);
  QCOMPARE(bytes, QByteArray::fromHex("913C64"));

  bytes.clear();
  append_note_off(bytes, 1, 60);
  QCOMPARE(bytes, QByteArray::fromHex("813C00"));

  bytes.clear();
  // 8192 is the centered (zero-bend) 14-bit value
  append_pitch_bend(bytes, 0, 8192);
  QCOMPARE(bytes, QByteArray::fromHex("E00040"));

  bytes.clear();
  append_be16(bytes, 300);
  QCOMPARE(bytes, QByteArray::fromHex("012C"));

  bytes.clear();
  append_chunk(bytes, "MThd", QByteArray("XY"));
  QCOMPARE(bytes, QByteArray("MThd") + QByteArray::fromHex("00000002") + QByteArray("XY"));
}

// exercises MidiTrackEvent::write's std::visit dispatch (and each
// *EventInfo::write member) directly, rather than only through the
// free encoding functions above
void Tester::test_midi_track_event_write_dispatch() {
  QByteArray bytes;

  MidiTrackEvent{
      .tick = 0, .tie_break = 0,
      .info = TempoEventInfo{.microseconds_per_quarter = 500000}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("FF510307A120"));

  bytes.clear();
  MidiTrackEvent{.tick = 0, .tie_break = 0,
                 .info = TrackNameEventInfo{.name = "Hi"}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("FF0302") + QByteArray("Hi"));

  bytes.clear();
  MidiTrackEvent{
      .tick = 0, .tie_break = 0,
      .info = ProgramChangeEventInfo{.channel_number = 2, .program_number = 5}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("C205"));

  bytes.clear();
  MidiTrackEvent{
      .tick = 0, .tie_break = 0,
      .info = PitchBendEventInfo{.channel_number = 0, .bend_14_bit = 8192}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("E00040"));

  bytes.clear();
  MidiTrackEvent{.tick = 0, .tie_break = 0,
                 .info = ControlChangeEventInfo{.channel_number = 3,
                                                .controller = 7,
                                                .value = 100}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("B30764"));

  bytes.clear();
  MidiTrackEvent{
      .tick = 0, .tie_break = 0,
      .info = NoteOnEventInfo{
          .channel_number = 1, .midi_number = 60, .velocity = 100}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("913C64"));

  bytes.clear();
  MidiTrackEvent{.tick = 0, .tie_break = 0,
                 .info = NoteOffEventInfo{.channel_number = 1,
                                          .midi_number = 60}}
      .write(bytes);
  QCOMPARE(bytes, QByteArray::fromHex("813C00"));
}
