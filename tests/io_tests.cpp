#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QtTypes>
#include <QtTest/QTest>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>
#include <fluidsynth.h>
#include <fluidsynth/audio.h>
#include <fluidsynth/types.h>
#include <limits>
#include <string>
#include <utility>
#include <zip.h>
#include <zipconf.h>

#include "Tester.hpp"
#include "other/helpers.hpp"
#include "sound/FluidDriver.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/FluidSynth.hpp"
#include "sound/Player.hpp"
#include "test_helpers.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/ZipArchive.hpp"

// regression test: FluidDriver's move-assignment operator must free any
// audio driver it already owns before taking on a new one, and must be a
// no-op on self-move-assignment -- the original bug overwrote
// internal_pointer unconditionally, which would leak a live driver on
// reassignment and, on self-move specifically, null out internal_pointer
// without ever freeing it, losing the handle entirely
void Tester::test_fluid_driver_move_assign() {
  FluidSettings settings;
#ifdef __linux__
  set_fluid_string(settings, "audio.driver", "pulseaudio");
#endif
  FluidSynth synth(settings);
  auto *const audio_driver_pointer = new_fluid_audio_driver(
      settings.internal_pointer, synth.internal_pointer);
  if (audio_driver_pointer == nullptr) {
    QSKIP("no audio driver available in this environment");
  }
  FluidDriver driver(audio_driver_pointer);

  // an intermediate reference keeps this a genuine self-move at runtime
  // without the literal "driver = std::move(driver)" syntax that trips
  // -Wself-move
  auto &driver_ref = driver;
  driver = std::move(driver_ref);
  QCOMPARE(driver.internal_pointer, audio_driver_pointer);

  FluidDriver empty_driver(nullptr);
  driver = std::move(empty_driver);
  QCOMPARE(driver.internal_pointer,
           static_cast<fluid_audio_driver_t *>(nullptr));
  QCOMPARE(empty_driver.internal_pointer,
           static_cast<fluid_audio_driver_t *>(nullptr));
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
      << ZIP_STAT_SIZE << zip_uint64_t{13}
      << true;
  QTest::newRow("largest int-sized entry")
      << ZIP_STAT_SIZE
      << static_cast<zip_uint64_t>(std::numeric_limits<int>::max())
      << true;
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
      << static_cast<qsizetype>(std::numeric_limits<int>::max()) + 1
      << false;
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
