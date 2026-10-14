#include <QtGui/QFileOpenEvent>

#include "Tester.hpp"
#include "io/FileOpener.hpp"

// regression test: CHandle's (here, FluidDriver's) move-assignment must free
// any audio driver it already owns before taking on a new one, and must be a
// no-op on self-move-assignment -- the original bug overwrote
// internal_pointer unconditionally, which would leak a live driver on
// reassignment and, on self-move specifically, null out internal_pointer
// without ever freeing it, losing the handle entirely
void Tester::test_fluid_driver_move_assign() {
  // the "file" audio driver renders to a file instead of a sound card, so
  // this doesn't depend on the environment having an audio backend
  const QTemporaryDir temp_dir;
  QVERIFY(temp_dir.isValid());
  FluidSettings settings;
  set_fluid_string(settings, "audio.driver", "file");
  set_fluid_string(settings, "audio.file.name",
                   temp_dir.filePath("driver.wav").toStdString().c_str());
  const FluidSynth synth(settings);
  auto* const audio_driver_pointer =
      new_fluid_audio_driver(settings.internal_pointer, synth.internal_pointer);
  if (audio_driver_pointer == nullptr) {
    QSKIP("no audio driver available in this environment");
  }
  // the move constructor is never reached through Player (guaranteed copy
  // elision constructs make_audio_driver's result in place), so exercise it
  // directly
  FluidDriver source_driver(audio_driver_pointer);
  FluidDriver driver(std::move(source_driver));
  QCOMPARE(driver.internal_pointer, audio_driver_pointer);
  // checking the moved-from state is the point of this test
  // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
  QCOMPARE(source_driver.internal_pointer,
           static_cast<fluid_audio_driver_t*>(nullptr));

  // an intermediate reference keeps this a genuine self-move at runtime
  // without the literal "driver = std::move(driver)" syntax that trips
  // -Wself-move
  auto& driver_ref = driver;
  driver = std::move(driver_ref);
  QCOMPARE(driver.internal_pointer, audio_driver_pointer);

  FluidDriver empty_driver(nullptr);
  driver = std::move(empty_driver);
  QCOMPARE(driver.internal_pointer,
           static_cast<fluid_audio_driver_t*>(nullptr));
  // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved)
  QCOMPARE(empty_driver.internal_pointer,
           static_cast<fluid_audio_driver_t*>(nullptr));
}

// regression test: read_xml_document casts a QByteArray's size down to int
// before handing it to xmlReadMemory, but xmlReadMemory reads however many
// bytes the (uncast) length claims -- a buffer whose size doesn't fit in an
// int must be rejected up front instead of under-reporting its length
void Tester::test_xml_bytes_size_is_safe_data() {
  QTest::addColumn<qsizetype>("size");
  QTest::addColumn<bool>("is_safe");

  QTest::newRow("ordinary small buffer") << qsizetype{13} << true;
  QTest::newRow("largest int-sized buffer")
      << static_cast<qsizetype>(std::numeric_limits<int>::max()) << true;
  QTest::newRow("just over int-sized buffer")
      << static_cast<qsizetype>(std::numeric_limits<int>::max()) + 1 << false;
}

void Tester::test_xml_bytes_size_is_safe() {
  QFETCH(const qsizetype, size);
  QFETCH(const bool, is_safe);

  QCOMPARE(xml_bytes_size_is_safe(size), is_safe);
}

// get_share_file's missing-file path (Q_ASSERT compiles out in release
// builds, so a broken/incomplete installation missing a bundled resource
// -- an xsd schema, the icon, the soundfont -- must still be rejected
// regardless of build type) now shows QMessageBox::critical and calls
// std::exit(), so it can't be exercised from within this test binary
// without killing the whole run; not covered here.
void Tester::test_get_share_file_existing() const {
  QCOMPARE(get_share_file("Justly.svg"),
           test_dir.filePath("Justly.svg").toStdString());
}

void Tester::test_open_error_data() {
  QTest::addColumn<QString>("text");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("not xml") << "<" << "Invalid XML file";
  QTest::newRow("not Justly") << "<song/>" << "Invalid song file";
}

void Tester::test_open_error() {
  QFETCH(const QString, text);
  QFETCH(const QString, error_message);

  close_message_later(main_window, waiting_for_message, error_message);
  open_text(main_window, text);
}

// every file dialog leaves the song alone when cancelled -- the complement
// of the accept paths driven by test_open_via_dialog, test_import_via_dialog
// and test_export_via_dialog
void Tester::test_file_dialog_reject_data() {
  QTest::addColumn<QString>("action_text");

  QTest::newRow("open") << "&Open";
  QTest::newRow("import") << "&Import MusicXML";
  QTest::newRow("export") << "&Export recording";
}

void Tester::test_file_dialog_reject() {
  QFETCH(const QString, action_text);

  auto& window_body = main_window.window_body;
  const auto actions = main_window.song_menu_bar.file_menu.actions();
  const auto action_iterator = std::ranges::find_if(
      actions, [&action_text](const QAction* const action_pointer) -> auto {
        return action_pointer->text() == action_text;
      });
  QVERIFY(action_iterator != actions.cend());

  const auto old_current_file = window_body.current_file;
  const auto old_number_of_chords = window_body.song.chords.size();

  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&main_window));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &main_window, []() -> auto {
    auto* const found_dialog = find_top_level_file_dialog();
    QVERIFY(found_dialog != nullptr);
    found_dialog->reject();
  });
  timer.start(WAIT_TIME);

  get_reference(*action_iterator).trigger();

  QCOMPARE(window_body.current_file, old_current_file);
  QCOMPARE(window_body.song.chords.size(), old_number_of_chords);
}

void Tester::test_open_via_dialog() {
  auto& window_body = main_window.window_body;
  const auto fixture_file = test_dir.filePath("test_song.justly");

  // start from a different song, so reopening the fixture visibly replaces it
  open_text(main_window, make_voice_song_xml({"A"}, {"D"}, {{{0}, {}}}));
  QCOMPARE(window_body.song.chords.size(), 1);

  accept_file_dialog_later(main_window, fixture_file);
  main_window.song_menu_bar.file_menu.open_action.trigger();

  QCOMPARE(window_body.current_file, fixture_file);
  QCOMPARE_NE(window_body.song.chords.size(), 1);
}

// text with XML special characters must be escaped on save, or reopening
// would lose or reject it
void Tester::test_save_escapes_text() {
  auto& window_body = main_window.window_body;
  auto& song = window_body.song;

  open_text(main_window,
            make_voice_song_xml({"R&amp;B &lt;1&gt;"}, {"D"}, {{{0}, {}}})
                .replace("</voice_name></pitched_note>",
                         "</voice_name><words>a &amp; b &lt; c</words>"
                         "</pitched_note>"));

  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.close();
  save_as_file(window_body, temp_file.fileName());
  QVERIFY(open_file(main_window, temp_file.fileName()));

  QCOMPARE(song.pitched_voices.at(0).name, QString("R&B <1>"));
  const auto& note = song.chords.at(0).pitched_notes.at(0);
  QCOMPARE(note.voice_name, QString("R&B <1>"));
  QCOMPARE(note.words, QString("a & b < c"));

  // restore the shared fixture
  open_file(main_window, test_dir.filePath("test_song.justly"));
}

void Tester::test_import_via_dialog() {
  auto& switch_table = main_window.window_body.switch_column.switch_table;

  accept_file_dialog_later(main_window,
                           test_dir.filePath("percussion.musicxml"));
  main_window.song_menu_bar.file_menu.import_action.trigger();

  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), PERCUSSION_ROWS);

  // restore the shared fixture
  open_file(main_window, test_dir.filePath("test_song.justly"));
}

void Tester::test_open_asks_to_discard_changes_data() {
  QTest::addColumn<QString>("action_text");
  QTest::addColumn<bool>("discard");

  QTest::newRow("open, keep changes") << "&Open" << false;
  QTest::newRow("open, discard changes") << "&Open" << true;
  QTest::newRow("import, keep changes") << "&Import MusicXML" << false;
  QTest::newRow("import, discard changes") << "&Import MusicXML" << true;
}

// opening or importing over unsaved changes asks first, and only goes on to
// pick a file once the user agrees to discard them
void Tester::test_open_asks_to_discard_changes() {
  QFETCH(const QString, action_text);
  QFETCH(const bool, discard);

  auto& window_body = main_window.window_body;
  auto& undo_stack = window_body.undo_stack;
  const auto actions = main_window.song_menu_bar.file_menu.actions();
  const auto action_iterator = std::ranges::find_if(
      actions, [&action_text](const QAction* const action_pointer) -> auto {
        return action_pointer->text() == action_text;
      });
  QVERIFY(action_iterator != actions.cend());

  select_cell(window_body.switch_column.switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
  QVERIFY(!undo_stack.isClean());
  const auto number_of_chords = window_body.song.chords.size();

  answer_question_later(main_window, waiting_for_message,
                        "Discard unsaved changes?",
                        discard ? QMessageBox::Yes : QMessageBox::No);
  // only shows up (and then gets cancelled) if the changes were discarded
  auto dialog_shown = false;
  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&main_window));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &main_window,
                   [&dialog_shown]() -> auto {
                     auto* const found_dialog = find_top_level_file_dialog();
                     if (found_dialog != nullptr) {
                       dialog_shown = true;
                       found_dialog->reject();
                     }
                   });
  timer.start(WAIT_TIME * 2);

  get_reference(*action_iterator).trigger();
  if (!discard) {
    // give the dialog timer its chance to find nothing
    QTest::qWait(WAIT_TIME * 3);
  }

  QCOMPARE(dialog_shown, discard);
  QCOMPARE(window_body.song.chords.size(), number_of_chords);
  undo_stack.undo();
  QVERIFY(undo_stack.isClean());
}

// a file from the command line, e.g. a desktop's "Open With", is imported
// or opened by its extension, after asking about unsaved changes
void Tester::test_open_or_import() {
  auto& window_body = main_window.window_body;
  auto& undo_stack = window_body.undo_stack;
  const auto song_file = test_dir.filePath("test_song.justly");

  QVERIFY(open_or_import(main_window, test_dir.filePath("prelude.musicxml")));
  QCOMPARE(window_body.song.chords.size(), MUSIC_XML_ROWS);

  QVERIFY(open_or_import(main_window, song_file));
  QCOMPARE(window_body.current_file, song_file);

  select_cell(window_body.switch_column.switch_table, 0, 0);
  main_window.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
  QVERIFY(!undo_stack.isClean());
  answer_question_later(main_window, waiting_for_message,
                        "Discard unsaved changes?", QMessageBox::No);
  QVERIFY(!open_or_import(main_window, test_dir.filePath("prelude.musicxml")));
  QVERIFY(!undo_stack.isClean());

  // restore the shared fixture
  open_file(main_window, song_file);
}

// a file the system asks Justly to open waits for the recovery prompt, then
// opens; on macOS, it comes in a file open event
void Tester::test_file_opener() {
  auto& window_body = main_window.window_body;
  const auto song_file = test_dir.filePath("test_song.justly");

  FileOpener file_opener(main_window);
  QCoreApplication::instance()->installEventFilter(&file_opener);
  QFileOpenEvent file_open_event(test_dir.filePath("prelude.musicxml"));
  QCoreApplication::sendEvent(QCoreApplication::instance(), &file_open_event);
  QCOMPARE(window_body.current_file, song_file);
  QVERIFY(file_opener.pending_file.has_value());

  file_opener.start();
  QCOMPARE(window_body.song.chords.size(), MUSIC_XML_ROWS);
  QVERIFY(!file_opener.pending_file.has_value());

  // once started, opens right away
  file_opener.open(song_file);
  QCOMPARE(window_body.current_file, song_file);
  QCoreApplication::instance()->removeEventFilter(&file_opener);
}
