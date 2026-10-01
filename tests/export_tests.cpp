#ifdef __linux__
#include <sys/resource.h>

#include <csignal>
#endif

#include "Tester.hpp"
#include "other/MidiTrackEvent.hpp"

void Tester::test_export() {
  auto& window_body = main_window.window_body;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(window_body, temp_export_file.fileName());
}

void Tester::test_export_midi() {
  auto& window_body = main_window.window_body;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  // the fixture's chord 2 has three different unpitched voices starting
  // at the same tick, which can't all occupy GM's single shared
  // percussion channel at once -- export aborts on the first conflict
  // rather than silently writing a file missing notes the user didn't
  // ask to drop
  close_message_later(
      main_window, waiting_for_message,
      "Percussion instrument Room for chord 2, unpitched note 2 starts "
      "at the same time as a different percussion instrument on the "
      "shared MIDI percussion channel");
  export_midi_to_file(window_body, temp_export_file.fileName());

  QFile written_file(temp_export_file.fileName());
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QCOMPARE(written_file.read(4), QByteArray());
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

// regression test: same gap as test_export_via_dialog, but for
// export_midi_action. Accepting the dialog triggers export_midi_to_file,
// which (like test_export_midi) hits the fixture's percussion-channel
// conflict and pops up a second, nested modal dialog -- so the file
// dialog is accepted after a short delay, and the resulting message box
// is closed by a separately-armed timer that fires later
void Tester::test_export_midi_via_dialog() {
  auto& song_menu_bar = main_window.song_menu_bar;

  // see test_export_via_dialog: a path that doesn't exist yet, so
  // accept() doesn't pop up an overwrite-confirmation box on top of the
  // percussion-conflict warning this test already expects
  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  auto export_filename = temp_export_dir.filePath("export.mid");

  close_message_later(
      main_window, waiting_for_message,
      "Percussion instrument Room for chord 2, unpitched note 2 starts "
      "at the same time as a different percussion instrument on the "
      "shared MIDI percussion channel");

  auto& dialog_timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&main_window));
  dialog_timer.setSingleShot(true);
  QObject::connect(&dialog_timer, &QTimer::timeout, &main_window,
                   [export_filename]() -> auto {
                     auto* const found_dialog = find_top_level_file_dialog();
                     QVERIFY(found_dialog != nullptr);
                     // see accept_file_dialog_later: type into the
                     // filename line edit directly rather than using
                     // selectFile(), which only sets the directory for a
                     // not-yet-existing target
                     auto* const line_edit =
                         found_dialog->findChild<QLineEdit*>();
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
// Open/Import/Save As/Export/Export MIDI used to create a new QFileDialog
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

// test_export_midi_success only writes small values, so exercise the
// multi-byte variable-length encoding directly
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

namespace {

// one pitched voice, one unpitched voice, and a single chord holding the
// given notes (an empty string omits that kind of note from the chord)
auto make_export_song_xml(const int starting_velocity,
                          const QString& pitched_note_fields,
                          const QString& unpitched_note_fields) -> QString {
  QString body = R"(
  <pitched_voices>
    <pitched_voice>
      <name>A</name>
      <instrument>Marimba</instrument>
    </pitched_voice>
  </pitched_voices>
  <unpitched_voices>
    <unpitched_voice>
      <name>D</name>
      <percussion_set_pointer>Room</percussion_set_pointer>
      <midi_number>36</midi_number>
    </unpitched_voice>
  </unpitched_voices>
  <chords>
    <chord>)";
  if (!pitched_note_fields.isEmpty()) {
    body += R"(
      <pitched_notes>
        <pitched_note>
          <voice_name>A</voice_name>
          )" +
            pitched_note_fields +
            R"(
        </pitched_note>
      </pitched_notes>)";
  }
  if (!unpitched_note_fields.isEmpty()) {
    body += R"(
      <unpitched_notes>
        <unpitched_note>
          <voice_name>D</voice_name>
          )" +
            unpitched_note_fields +
            R"(
        </unpitched_note>
      </unpitched_notes>)";
  }
  body += R"(
    </chord>
  </chords>)";
  return make_song_xml(starting_velocity, body);
}

auto get_plain_words() -> QString { return "<words>n</words>"; }

auto get_double_velocity() -> QString {
  return "<velocity_ratio><numerator>2</numerator></velocity_ratio>";
}

}  // namespace

void Tester::test_export_midi_error_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("pitched velocity too high")
      << make_export_song_xml(100, get_double_velocity(), "")
      << "Velocity 200 exceeds 127 for chord 1, pitched note 1";
  QTest::newRow("unpitched velocity too high")
      << make_export_song_xml(100, "", get_double_velocity())
      << "Velocity 200 exceeds 127 for chord 1, unpitched note 1";
  QTest::newRow("frequency too high")
      << make_export_song_xml(10, "<interval><octave>9</octave></interval>", "")
      << "Frequency 1.13e+05 for chord 1, pitched note 1 is out of MIDI "
         "export range";
  QTest::newRow("frequency too low")
      << make_export_song_xml(10, "<interval><octave>-9</octave></interval>",
                              "")
      << "Frequency 0.43 for chord 1, pitched note 1 is out of MIDI export "
         "range";
}

void Tester::test_export_midi_error() {
  QFETCH(const QString, text);
  QFETCH(const QString, error_message);

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();

  open_text(main_window, text);
  close_message_later(main_window, waiting_for_message, error_message);
  export_midi_to_file(main_window.window_body, temp_export_file.fileName());

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_export_midi_success() {
  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  const auto export_filename = temp_export_dir.filePath("export.mid");

  // a 5/4 interval off the 220 Hz starting key lands between MIDI notes,
  // so the pitched note needs a pitch bend
  open_text(main_window,
            make_export_song_xml(
                10,
                "<interval><ratio><numerator>5</numerator><denominator>4"
                "</denominator></ratio></interval>",
                get_plain_words()));
  export_midi_to_file(main_window.window_body, export_filename);

  QFile written_file(export_filename);
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QCOMPARE(written_file.readAll(),
           QByteArray::fromHex(
               // header: format 1, 3 tracks, 500 ticks per quarter
               "4D546864"
               "00000006"
               "0001"
               "0003"
               "01F4"
               // tempo track
               "4D54726B"
               "0000000B"
               "00FF510307A120"  // 500000 microseconds per quarter
               "00FF2F00"        // end of track
               // pitched voice track
               "4D54726B"
               "00000019"
               "00FF030141"  // track name "A"
               "00C00C"      // channel 0: Marimba
               // 5/4 above 220 Hz is MIDI 60.86, so MIDI 61 bent down to
               // (60.86 - 61 + 2) * 4096 = 7631
               "00E04F3B"
               "00903D0A"    // note on, velocity 10
               "8458803D00"  // note off 600 ticks (1 beat at 100 bpm) later
               "00FF2F00"
               // unpitched voice track
               "4D54726B"
               "00000019"
               "00FF030144"  // track name "D"
               "00B90078"    // percussion channel 9: GM2 percussion bank
               "00C908"      // Room
               "0099240A"    // note on, MIDI 36, velocity 10
               "8458892400"
               "00FF2F00"));

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

void Tester::test_export_midi_unwritable_path() {
  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  const auto unwritable_path =
      temp_export_dir.filePath("nonexistent_subdir/export.mid");

  open_text(main_window,
            make_export_song_xml(10, get_plain_words(), get_plain_words()));
  close_message_later(main_window, waiting_for_message,
                      "Cannot open file for writing");
  export_midi_to_file(main_window.window_body, unwritable_path);
  QVERIFY(!QFile::exists(unwritable_path));

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// unpitched notes from the same percussion set share the percussion
// channel, so they can start together without a conflict, and any percussion
// can follow later on
void Tester::test_export_midi_shared_percussion_set() {
  const QTemporaryDir temp_export_dir;
  QVERIFY(temp_export_dir.isValid());
  const auto export_filename = temp_export_dir.filePath("export.mid");

  open_text(main_window,
            make_voice_song_xml({"A"}, {"D", "E"}, {{{}, {0, 1}}, {{}, {0}}}));
  export_midi_to_file(main_window.window_body, export_filename);

  QFile written_file(export_filename);
  QVERIFY(written_file.open(QIODevice::ReadOnly));
  QCOMPARE(written_file.read(4), QByteArray("MThd"));
  // one tempo track plus one track per voice
  QCOMPARE(QString::fromLatin1(written_file.readAll()).count("MTrk"), 4);

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}

// a MIDI file only has 15 non-percussion channels, so a 16th pitched note
// sounding at once can't be exported
void Tester::test_export_midi_channel_exhausted() {
  static const auto TOO_MANY_PITCHED_NOTES = 16;

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();

  open_text(main_window,
            make_voice_song_xml({"A"}, {"D"},
                                {{QList<int>(TOO_MANY_PITCHED_NOTES, 0), {}}}));
  close_message_later(main_window, waiting_for_message,
                      "More notes are sounding at once than there are "
                      "available MIDI channels");
  export_midi_to_file(main_window.window_body, temp_export_file.fileName());

  // restore the shared fixture
  open_file_and_reload(main_window.song_menu_bar, main_window.window_body,
                       main_window.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
}
