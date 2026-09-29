#include "Tester.hpp"
#include "xml/ZipArchive.hpp"

// regression test: FluidDriver's move-assignment operator must free any
// audio driver it already owns before taking on a new one, and must be a
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

// regression test: read_zip_entry casts a zip entry's reported size down
// to int before allocating its buffer, but reads however many bytes the
// (uncast, 64-bit) size claims -- an entry whose declared size doesn't fit
// in an int, or whose size libzip couldn't report at all, must be rejected
// up front instead of under-allocating the destination buffer
void Tester::test_zip_entry_size_is_safe_data() {
  QTest::addColumn<unsigned int>("valid_flags");
  QTest::addColumn<zip_uint64_t>("size");
  QTest::addColumn<bool>("is_safe");

  QTest::newRow("ordinary small entry")
      << ZIP_STAT_SIZE << zip_uint64_t{13} << true;
  QTest::newRow("largest int-sized entry")
      << ZIP_STAT_SIZE
      << static_cast<zip_uint64_t>(std::numeric_limits<int>::max()) << true;
  QTest::newRow("just over int-sized entry")
      << ZIP_STAT_SIZE
      << static_cast<zip_uint64_t>(std::numeric_limits<int>::max()) + 1
      << false;
  QTest::newRow("size libzip couldn't report")
      << static_cast<unsigned int>(0) << zip_uint64_t{13} << false;
}

void Tester::test_zip_entry_size_is_safe() {
  QFETCH(const unsigned int, valid_flags);
  QFETCH(const zip_uint64_t, size);
  QFETCH(const bool, is_safe);

  zip_stat_t entry_stat;
  zip_stat_init(&entry_stat);
  entry_stat.valid = valid_flags;
  entry_stat.size = size;

  QCOMPARE(zip_entry_size_is_safe(entry_stat), is_safe);
}

// regression test: read_zip_entry used to only Q_ASSERT that
// internal_pointer wasn't null before dereferencing it -- an assert that
// compiles away in release builds. A ZipArchive constructed from a file
// that doesn't exist (or isn't a zip) leaves internal_pointer null, and
// read_zip_entry must degrade to its documented "empty QByteArray" return
// instead of crashing.
void Tester::test_read_zip_entry_null_archive() const {
  const ZipArchive archive(test_dir.filePath("does_not_exist.zip"));
  QCOMPARE(archive.internal_pointer, nullptr);
  QCOMPARE(read_zip_entry(archive, "anything"), QByteArray());
}

// every way read_zip_entry can fail on an archive that opened fine must give
// the documented empty QByteArray instead of a partial or garbage buffer
void Tester::test_read_zip_entry_error_data() {
  QTest::addColumn<QString>("archive_name");
  QTest::addColumn<QString>("entry_name");

  QTest::newRow("missing entry")
      << "prelude.mxl" << "does_not_exist.xml";
  // the central directory claims a size that doesn't fit in an int
  QTest::newRow("oversized entry")
      << "zip_oversized_entry.zip" << "a.txt";
  // the entry is flagged as encrypted and no password is available, so it
  // shows up in the archive but can't be opened
  QTest::newRow("encrypted entry")
      << "zip_encrypted_entry.zip" << "a.txt";
  // the entry's data ends before the size the central directory claims
  QTest::newRow("short entry")
      << "zip_short_entry.zip" << "a.txt";
}

void Tester::test_read_zip_entry_error() const {
  QFETCH(const QString, archive_name);
  QFETCH(const QString, entry_name);

  const ZipArchive archive(test_dir.filePath(archive_name));
  QVERIFY(archive.internal_pointer != nullptr);
  QCOMPARE(read_zip_entry(archive, entry_name.toStdString()), QByteArray());
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

  close_message_later(song_editor, waiting_for_message, error_message);
  open_text(song_editor, text);
}

// every file dialog leaves the song alone when cancelled -- the complement
// of the accept paths driven by test_open_via_dialog, test_import_via_dialog
// and test_export_via_dialog
void Tester::test_file_dialog_reject_data() {
  QTest::addColumn<QString>("action_text");

  QTest::newRow("open") << "&Open";
  QTest::newRow("import") << "&Import MusicXML";
  QTest::newRow("export") << "&Export recording";
  QTest::newRow("export MIDI") << "Export &MIDI";
}

void Tester::test_file_dialog_reject() {
  QFETCH(const QString, action_text);

  auto& song_widget = song_editor.song_widget;
  const auto actions = song_editor.song_menu_bar.file_menu.actions();
  const auto action_iterator = std::ranges::find_if(
      actions, [&action_text](const QAction* const action_pointer) -> auto {
        return action_pointer->text() == action_text;
      });
  QVERIFY(action_iterator != actions.cend());

  const auto old_current_file = song_widget.current_file;
  const auto old_number_of_chords = song_widget.song.chords.size();

  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&song_editor));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &song_editor, []() -> auto {
    auto* const found_dialog = find_top_level_file_dialog();
    QVERIFY(found_dialog != nullptr);
    found_dialog->reject();
  });
  timer.start(WAIT_TIME);

  get_reference(*action_iterator).trigger();

  QCOMPARE(song_widget.current_file, old_current_file);
  QCOMPARE(song_widget.song.chords.size(), old_number_of_chords);
}

void Tester::test_open_via_dialog() {
  auto& song_widget = song_editor.song_widget;
  const auto fixture_file = test_dir.filePath("test_song.xml");

  // start from a different song, so reopening the fixture visibly replaces it
  open_text(song_editor, make_voice_song_xml({"A"}, {"D"}, {{{0}, {}}}));
  QCOMPARE(song_widget.song.chords.size(), 1);

  accept_file_dialog_later(song_editor, fixture_file);
  song_editor.song_menu_bar.file_menu.open_action.trigger();

  QCOMPARE(song_widget.current_file, fixture_file);
  QCOMPARE_NE(song_widget.song.chords.size(), 1);
}

void Tester::test_import_via_dialog() {
  auto& switch_table = song_editor.song_widget.switch_column.switch_table;

  accept_file_dialog_later(song_editor,
                           test_dir.filePath("percussion.musicxml"));
  song_editor.song_menu_bar.file_menu.import_action.trigger();

  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), PERCUSSION_ROWS);

  // restore the shared fixture
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));
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

  auto& song_widget = song_editor.song_widget;
  auto& undo_stack = song_widget.undo_stack;
  const auto actions = song_editor.song_menu_bar.file_menu.actions();
  const auto action_iterator = std::ranges::find_if(
      actions, [&action_text](const QAction* const action_pointer) -> auto {
        return action_pointer->text() == action_text;
      });
  QVERIFY(action_iterator != actions.cend());

  select_cell(song_widget.switch_column.switch_table, 0, 0);
  song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action.trigger();
  QVERIFY(!undo_stack.isClean());
  const auto number_of_chords = song_widget.song.chords.size();

  answer_question_later(song_editor, waiting_for_message,
                        "Discard unsaved changes?",
                        discard ? QMessageBox::Yes : QMessageBox::No);
  // only shows up (and then gets cancelled) if the changes were discarded
  auto dialog_shown = false;
  auto& timer =  // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&song_editor));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &song_editor,
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
  QCOMPARE(song_widget.song.chords.size(), number_of_chords);
  undo_stack.undo();
  QVERIFY(undo_stack.isClean());
}
