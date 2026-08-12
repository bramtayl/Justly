#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QFlags>
#include <QtCore/QIODevice>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QLineF>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QPointer>
#include <QtCore/QRect>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtAssert>
#include <QtCore/QtTypes>
#include <QtCore/qtmetamacros.h>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QPen>
#include <QtGui/QTransform>
#include <QtGui/QUndoStack>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestkeyboard.h>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>
#include <algorithm>
#include <fluidsynth.h>
#include <fluidsynth/audio.h>
#include <fluidsynth/types.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <zip.h>
#include <zipconf.h>

#include "cell_types/Interval.hpp"
#include "cell_types/Rational.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/EditMenu.hpp"
#include "menus/FileMenu.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "models/ChordsModel.hpp"
#include "musicxml/MeasureRepeatInfo.hpp"
#include "other/PianoRollNoteEvent.hpp"
#include "other/Song.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "sound/FluidDriver.hpp"
#include "sound/FluidSettings.hpp"
#include "sound/Player.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchDelegate.hpp"
#include "widgets/SwitchTable.hpp"
#include "widgets/piano_roll/PianoRollNotesView.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"
#include "xml/XMLDocument.hpp"
#include "xml/ZipArchive.hpp"

static const auto BIG_VELOCITY = 126;
static const auto PERCUSSION_ROWS = 16;
static const auto MOZART_ROWS = 147;
static const auto SALTARELLO_ROWS = 184;
static const auto MUSIC_XML_ROWS = 545;
static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;
static const auto RATIO_SHIFT_TIMES = 6;
static const auto OCTAVE_SHIFT_TIMES = 9;
static const auto SELECT_AND_CLEAR =
    QItemSelectionModel::Select | QItemSelectionModel::Clear;
static const auto STARTING_KEY_1 = 401.0;
static const auto STARTING_KEY_2 = 402.0;
static const auto STARTING_TEMPO_1 = 150.0;
static const auto STARTING_TEMPO_2 = 125.0;
static const auto STARTING_VELOCITY_1 = 70.0;
static const auto STARTING_VELOCITY_2 = 80.0;
static const auto WAIT_TIME = 500;

static const auto RECOVERY_PROMPT_TEXT =
    "Justly didn't close properly last time. Restore the unsaved work "
    "from your last session?";

static const auto A_MINUS_FREQUENCY = 217;
static const auto A_PLUS_FREQUENCY = 223;
static const auto A_FREQUENCY = 220;
static const auto B_FLAT_FREQUENCY = 233;
static const auto B_FREQUENCY = 247;
static const auto C_FREQUENCY = 262;
static const auto C_SHARP_FREQUENCY = 277;
static const auto D_FREQUENCY = 294;
static const auto E_FLAT_FREQUENCY = 311;
static const auto E_FREQUENCY = 330;
static const auto F_FREQUENCY = 349;
static const auto F_SHARP_FREQUENCY = 370;
static const auto G_FREQUENCY = 392;
static const auto A_FLAT_FREQUENCY = 415;

static const auto SIX = 6;
static const auto EIGHT = 8;

static auto get_model(QAbstractItemView &table) -> auto & {
  return get_reference(table.model());
}

static void double_click_column(QAbstractItemView &table, const int row_number,
                                const int column_number) {
  table.doubleClicked(get_model(table).index(row_number, column_number));
}

static void select_cell(QAbstractItemView &table, const int row,
                        const int column) {
  get_selection_model(table).select(get_model(table).index(row, column),
                                    SELECT_AND_CLEAR);
}

static void switch_to(SongEditor &song_editor, const RowType row_type,
                      const int chord_number) {
  auto &switch_table = song_editor.song_widget.switch_column.switch_table;
  switch (row_type) {
  case RowType::chord_type:
    QVERIFY(chord_number == -1);
    break;
  case RowType::pitched_note_type:
    double_click_column(switch_table, chord_number,
                        static_cast<int>(ChordColumn::chord_pitched_notes_column));
    break;
  case RowType::unpitched_note_type:
    double_click_column(switch_table, chord_number,
                        static_cast<int>(ChordColumn::chord_unpitched_notes_column));
    break;
  case RowType::pitched_voice_type:
    QVERIFY(chord_number == -1);
    song_editor.song_menu_bar.view_menu.edit_pitched_voices_action.trigger();
    break;
  case RowType::unpitched_voice_type:
    QVERIFY(chord_number == -1);
    song_editor.song_menu_bar.view_menu.edit_unpitched_voices_action.trigger();
    break;
  }
}

static void maybe_switch_back_to_chords(QUndoStack &undo_stack,
                                        const RowType row_type) {
  if (row_type != RowType::chord_type) {
    undo_stack.undo();
  }
}

static void open_text(SongWidget &song_widget, const QString &song_text) {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(song_text.toStdString().c_str());
  temp_file.close();
  open_file(song_widget, temp_file.fileName());
}

[[nodiscard]] static auto find_top_level_message_box() -> QMessageBox * {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *const box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
    if (box_pointer != nullptr && box_pointer->isVisible()) {
      return box_pointer;
    }
  }
  return nullptr;
}

[[nodiscard]] static auto find_top_level_file_dialog() -> QFileDialog * {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *const dialog_pointer = dynamic_cast<QFileDialog *>(widget_pointer);
    if (dialog_pointer != nullptr && dialog_pointer->isVisible()) {
      return dialog_pointer;
    }
  }
  return nullptr;
}

static void close_message_later(QWidget &parent, bool &waiting_for_message,
                                const QString &expected_text) {
  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&parent));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &parent,
      [expected_text, &waiting_for_message]() -> auto {
        auto *const box_pointer = find_top_level_message_box();
        if (box_pointer != nullptr) {
          auto actual_text = box_pointer->text();
          waiting_for_message = false;
          QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
          QCOMPARE(actual_text, expected_text);
        }
      });
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);
};

// like close_message_later, but for an action that pops up several message
// boxes in a row (e.g. a voice removal that warns about a reassigned live
// note and then, separately, a reassigned clipboard entry); re-arms the
// timer after each box closes so every expected text gets matched in order
static void
close_messages_later(QWidget &parent, bool &waiting_for_message,
                     const QList<QString> &expected_texts) {
  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto remaining_texts =
      std::make_shared<QList<QString>>(expected_texts);
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&parent));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &parent,
      [remaining_texts, &waiting_for_message, &timer]() -> auto {
        auto *const box_pointer = find_top_level_message_box();
        if (box_pointer != nullptr) {
          const auto actual_text = box_pointer->text();
          QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
          QVERIFY(!remaining_texts->empty());
          QCOMPARE(actual_text, remaining_texts->takeFirst());
          if (remaining_texts->empty()) {
            waiting_for_message = false;
          } else {
            timer.start(WAIT_TIME);
          }
        }
      });
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);
};

// like close_message_later, but for a Yes/No QMessageBox::question -- clicks
// the given standard button instead of always accepting via Enter, so tests
// can drive either branch (e.g. the recovery-restore prompt)
static void answer_question_later(QWidget &parent, bool &waiting_for_message,
                                  const QString &expected_text,
                                  QMessageBox::StandardButton answer) {
  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&parent));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &parent,
      [expected_text, answer, &waiting_for_message]() -> auto {
        auto *const box_pointer = find_top_level_message_box();
        if (box_pointer != nullptr) {
          auto actual_text = box_pointer->text();
          waiting_for_message = false;
          get_reference(box_pointer->button(answer)).click();
          QCOMPARE(actual_text, expected_text);
        }
      });
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);
};

static void press_times(QPushButton &plus_button, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    plus_button.click();
  }
}

static void undo_times(QUndoStack &undo_stack, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    undo_stack.undo();
  }
}

static void add_table_columns() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("chord_number");
}

static void add_tables() {
  add_table_columns();

  QTest::newRow("chord") << RowType::chord_type << -1;
  QTest::newRow("pitched note") << RowType::pitched_note_type << 1;
  QTest::newRow("unpitched note") << RowType::unpitched_note_type << 1;
  QTest::newRow("pitched voice") << RowType::pitched_voice_type << -1;
  QTest::newRow("unpitched voice") << RowType::unpitched_voice_type << -1;
}

static void add_cells() {
  add_table_columns();
  QTest::addColumn<int>("row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_pitched_notes_column);
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1 << 1
      << static_cast<int>(ChordColumn::chord_unpitched_notes_column);
  QTest::newRow("chord interval")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_interval_column);
  QTest::newRow("chord beats")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_beats_column);
  QTest::newRow("chord velocity ratio")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_velocity_ratio_column);
  QTest::newRow("chord tempo ratio")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_tempo_ratio_column);
  QTest::newRow("chord words")
      << RowType::chord_type << -1 << 1 << static_cast<int>(ChordColumn::chord_words_column);
  QTest::newRow("pitched note voice")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("pitched note interval")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_interval_column);
  QTest::newRow("pitched note beats")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_beats_column);
  QTest::newRow("pitched note velocity ratio")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column);
  QTest::newRow("pitched note words")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_words_column);
  QTest::newRow("unpitched note voice")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column);
  QTest::newRow("unpitched note beats")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column);
  QTest::newRow("unpitched note velocity ratio")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column);
  QTest::newRow("unpitched note words")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column);
  // note: pitched/unpitched voice columns are deliberately not covered here.
  // paste_after/paste_into insert a brand new row built only from the
  // pasted column(s), so a voice inserted this way would have an empty
  // (invalid) name; delete is covered separately in test_delete_data, since
  // deleting a voice's non-name columns doesn't create a new row.
}

static void add_editable_cell_pairs() {
  add_table_columns();
  QTest::addColumn<int>("first_row_number");
  QTest::addColumn<int>("second_row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord interval")
      << RowType::chord_type << -1 << 0 << 1 << static_cast<int>(ChordColumn::chord_interval_column);
  QTest::newRow("chord beats")
      << RowType::chord_type << -1 << 0 << 1 << static_cast<int>(ChordColumn::chord_beats_column);
  QTest::newRow("chord velocity ratio")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_velocity_ratio_column);
  QTest::newRow("chord tempo ratio")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_tempo_ratio_column);
  QTest::newRow("chord words")
      << RowType::chord_type << -1 << 0 << 1 << static_cast<int>(ChordColumn::chord_words_column);
  QTest::newRow("pitched note voice")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("pitched note interval")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_interval_column);
  QTest::newRow("pitched note beats")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_beats_column);
  QTest::newRow("pitched note velocity ratio")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column);
  QTest::newRow("pitched note words")
      << RowType::pitched_note_type << 1 << 0 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_words_column);
  QTest::newRow("unpitched note voice")
      << RowType::unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column);
  QTest::newRow("unpitched note beats")
      << RowType::unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column);
  QTest::newRow("unpitched note velocity ratio")
      << RowType::unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column);
  QTest::newRow("unpitched note words")
      << RowType::unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column);
  // note: voice name columns are excluded here, since copy/cut/paste of
  // voice names is disabled (a voice's name must be typed, not copied, so
  // that it stays unique -- see ReplaceTable.hpp)
}

// pitched/unpitched voice columns other than name: unlike
// add_editable_cell_pairs's rows, row 0 doesn't hold each column's
// compile-time default value, so these can't be shared with test_cut (which
// checks that cutting produces the default) -- only with tests that just
// need two distinct existing values (test_copy, test_set_value)
static void add_voice_column_pairs() {
  QTest::newRow("pitched voice instrument")
      << RowType::pitched_voice_type << -1 << 0 << 1
      << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
  QTest::newRow("unpitched voice percussion set")
      << RowType::unpitched_voice_type << -1 << 0 << 1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
  QTest::newRow("unpitched voice midi number")
      << RowType::unpitched_voice_type << -1 << 0 << 1
      << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
}

static void add_cell_pairs() {
  add_editable_cell_pairs();
  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_pitched_notes_column);
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_unpitched_notes_column);
}

static auto get_file_text(const QString &filename) {
  QFile file(filename);
  Q_ASSERT(file.open(QIODevice::ReadOnly));
  QString file_text(file.readAll());
  file.close();
  // normalize line endings
  file_text.replace("\r\n", "\n");
  return file_text;
}

struct Tester : public QObject {
  Q_OBJECT
public:
  SongEditor song_editor;
  QDir test_dir = get_share_folder();
  bool waiting_for_message = false;
  // watches for a QMessageBox that pops up while no test is expecting one
  // (via close_message_later) and fails fast instead of leaving it open --
  // QMessageBox::exec() runs a nested event loop, so this timer still fires
  // while a test is blocked waiting on it
  QTimer unexpected_message_timer;

  Tester() {
    // redirects QStandardPaths::AppDataLocation and QSettings's default
    // storage (used by the crash-recovery feature) away from the real
    // per-user Justly config/data dirs, so running tests can't clobber a
    // recovery file left behind by a real crashed session
    QStandardPaths::setTestModeEnabled(true);
    set_up();
    const auto fixture_file = test_dir.filePath("test_song.xml");
    // fail fast here instead of letting open_file's "Invalid XML file"
    // QMessageBox block forever with no user around to dismiss it
    Q_ASSERT(QFile::exists(fixture_file));
    open_file(song_editor.song_widget, fixture_file);

    QObject::connect(&unexpected_message_timer, &QTimer::timeout, this,
                     [this]() -> auto {
                       if (waiting_for_message) {
                         return;
                       }
                       auto *const box_pointer = find_top_level_message_box();
                       if (box_pointer == nullptr) {
                         return;
                       }
                       const auto actual_text = box_pointer->text();
                       box_pointer->close();
                       QFAIL(qUtf8Printable(
                           QString("Unexpected message box: %1")
                               .arg(actual_text)));
                     });
    unexpected_message_timer.start(WAIT_TIME);
  }

private slots:
  // the OS clipboard is a process-global singleton that outlives any single
  // test, so a stale copy left over from an earlier test (e.g. a copied
  // voice_number cell) must not bleed into a later test's voice removal and
  // trigger an unexpected clipboard-reassignment warning
  static void init() { get_clipboard().clear(); }

  static void test_column_count_data() {
    add_table_columns();
    QTest::addColumn<int>("number_of_columns");

    QTest::newRow("chord") << RowType::chord_type << -1
                           << static_cast<int>(ChordColumn::number_of_chord_columns);
    QTest::newRow("pitched note")
        << RowType::pitched_note_type << 0
        << static_cast<int>(PitchedNoteColumn::number_of_pitched_note_columns);
    QTest::newRow("unpitched note")
        << RowType::unpitched_note_type << 0
        << static_cast<int>(UnpitchedNoteColumn::number_of_unpitched_note_columns);
    QTest::newRow("pitched voice")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::number_of_pitched_voice_columns);
    QTest::newRow("unpitched voice")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::number_of_unpitched_voice_columns);
  };

  void test_column_count() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, number_of_columns);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_model(switch_table).columnCount(), number_of_columns);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_column_header_data() {
    add_table_columns();
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QString>("column_name");

    QTest::newRow("chord interval")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_interval_column)
        << "Interval";
    QTest::newRow("chord beats")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_beats_column) << "Beats";
    QTest::newRow("chord velocity ratio")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("chord tempo ratio")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_tempo_ratio_column)
        << "Tempo ratio";
    QTest::newRow("chord words")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_words_column) << "Words";
    QTest::newRow("chord pitched notes")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_pitched_notes_column)
        << "Pitched notes";
    QTest::newRow("chord unpitched notes")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_unpitched_notes_column)
        << "Unpitched notes";
    QTest::newRow("pitched note voice")
        << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column)
        << "Voice";
    QTest::newRow("pitched note interval")
        << RowType::pitched_note_type << 1
        << static_cast<int>(PitchedNoteColumn::pitched_note_interval_column) << "Interval";
    QTest::newRow("pitched note beats")
        << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_beats_column)
        << "Beats";
    QTest::newRow("pitched note velocity ratio")
        << RowType::pitched_note_type << 1
        << static_cast<int>(PitchedNoteColumn::pitched_note_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("pitched note words")
        << RowType::pitched_note_type << 1 << static_cast<int>(PitchedNoteColumn::pitched_note_words_column)
        << "Words";
    QTest::newRow("unpitched note voice")
        << RowType::unpitched_note_type << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column)
        << "Voice";
    QTest::newRow("unpitched note beats")
        << RowType::unpitched_note_type << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_beats_column) << "Beats";
    QTest::newRow("unpitched note velocity ratio")
        << RowType::unpitched_note_type << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("unpitched note words")
        << RowType::unpitched_note_type << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_words_column) << "Words";
    QTest::newRow("pitched voice name")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column) << "Name";
    QTest::newRow("pitched voice instrument")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column) << "Instrument";
    QTest::newRow("pitched voice velocity ratio")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("unpitched voice name")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column) << "Name";
    QTest::newRow("unpitched voice percussion set")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column)
        << "Percussion set";
    QTest::newRow("unpitched voice midi number")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column)
        << "MIDI number";
    QTest::newRow("unpitched voice velocity ratio")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column)
        << "Velocity ratio";
  };

  void test_column_header() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, column_number);
    QFETCH(const QString, column_name);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_model(switch_table).headerData(column_number, Qt::Horizontal),
             column_name);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_copy_data() {
    add_cell_pairs();
    add_voice_column_pairs();
  };

  void test_copy() {

    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, first_row_number);
    QFETCH(const int, second_row_number);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    auto &selector = get_selection_model(switch_table);

    const auto &first_index = model.index(first_row_number, column_number);
    const auto &second_index = model.index(second_row_number, column_number);

    const auto first_value = first_index.data();
    const auto second_value = second_index.data();

    QCOMPARE_NE(first_value, second_value);

    selector.select(first_index, SELECT_AND_CLEAR);
    edit_menu.copy_action.trigger();

    selector.select(second_index, SELECT_AND_CLEAR);
    edit_menu.paste_menu.paste_over_action.trigger();

    QCOMPARE(second_index.data(), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_cut_data() { add_cell_pairs(); };

  void test_cut() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, first_row_number);
    QFETCH(const int, second_row_number);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    auto &selector = get_selection_model(switch_table);

    const auto &first_index = model.index(first_row_number, column_number);
    const auto &second_index = model.index(second_row_number, column_number);

    const auto first_value = first_index.data();
    const auto second_value = second_index.data();

    QCOMPARE_NE(first_value, second_value);

    selector.select(second_index, SELECT_AND_CLEAR);
    edit_menu.cut_action.trigger();

    QCOMPARE(second_index.data(), first_value);

    selector.select(first_index, SELECT_AND_CLEAR);
    edit_menu.paste_menu.paste_over_action.trigger();

    QCOMPARE(first_index.data(), second_value);
    undo_stack.undo();
    QCOMPARE(first_index.data(), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_delete_data() {
    add_cells();
    QTest::newRow("pitched voice name")
        << RowType::pitched_voice_type << -1 << 0
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column);
    QTest::newRow("pitched voice instrument")
        << RowType::pitched_voice_type << -1 << 0
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
    QTest::newRow("unpitched voice name")
        << RowType::unpitched_voice_type << -1 << 0
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column);
    QTest::newRow("unpitched voice percussion set")
        << RowType::unpitched_voice_type << -1 << 0
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
    QTest::newRow("unpitched voice midi number")
        << RowType::unpitched_voice_type << -1 << 0
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
  };

  void test_delete() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, row_number);
    QFETCH(const int, column_number);

    switch_to(song_editor, row_type, chord_number);

    const auto delete_index =
        get_model(switch_table).index(row_number, column_number);
    const auto &old_value = delete_index.data();

    get_selection_model(switch_table).select(delete_index, SELECT_AND_CLEAR);
    song_editor.song_menu_bar.edit_menu.delete_cells_action.trigger();

    QCOMPARE_NE(old_value, delete_index.data());
    undo_stack.undo();
    QCOMPARE(old_value, delete_index.data());

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_export() {
    auto &song_widget = song_editor.song_widget;

    QTemporaryFile temp_export_file;
    QVERIFY(temp_export_file.open());
    temp_export_file.close();
    export_to_file(song_widget, temp_export_file.fileName());
  };

  void test_export_midi() {
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
  };

  // regression test: FileMenu's dialogs (make_file_dialog) must not leak --
  // Open/Import/Save As/Export/Export MIDI used to create a new QFileDialog
  // with no matching deleteLater(), so every use of a file dialog left a
  // live QFileDialog parented to song_widget for the rest of the process
  void test_file_dialog_cleanup() {
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
  };

  static void test_flag_data() {
    add_table_columns();
    QTest::addColumn<int>("column_number");
    QTest::addColumn<bool>("is_editable");

    QTest::newRow("chord interval")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_interval_column) << true;
    QTest::newRow("chord pitched notes")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_pitched_notes_column)
        << false;
    QTest::newRow("chord unpitched notes")
        << RowType::chord_type << -1 << static_cast<int>(ChordColumn::chord_unpitched_notes_column)
        << false;
    QTest::newRow("pitched note") << RowType::pitched_note_type << 1 << 0 << true;
    QTest::newRow("unpitched note") << RowType::unpitched_note_type << 1 << 0 << true;
    QTest::newRow("pitched voice") << RowType::pitched_voice_type << -1 << 0 << true;
    QTest::newRow("unpitched voice") << RowType::unpitched_voice_type << -1 << 0 << true;
  };

  void test_flag() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, column_number);
    QFETCH(const bool, is_editable);

    const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_model(switch_table).index(0, column_number).flags(),
             is_editable ? uneditable_flags | Qt::ItemIsEditable
                         : uneditable_flags);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  void test_frequency_bound_data() {
    QTest::addColumn<QPushButton *>("button_pointer");
    QTest::addColumn<QString>("error_message");

    auto &octave_row = song_editor.song_widget.controls_column.octave_row;

    QTest::newRow("too high")
        << &octave_row.plus_button
        << "Frequency 3.38e+05 for chord 2, pitched note 1 greater than or equal "
           "to maximum frequency 1.29e+04";
    QTest::newRow("too_low") << &octave_row.minus_button
                             << "Frequency 1.29 for chord 2, pitched note 1 "
                                "less than minimum frequency 7.94";
  }

  void test_frequency_bound() {
    QFETCH(QPushButton *, button_pointer);
    QFETCH(const QString, error_message);
    auto &button = get_reference(button_pointer);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, RowType::pitched_note_type, 1);
    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message, error_message);
    press_times(button, OCTAVE_SHIFT_TIMES);
    song_editor.song_menu_bar.play_menu.play_action.trigger();
    undo_times(song_widget.undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave
    maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
  };

  static void test_frequency_in_status_data() {
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
  };

  void test_frequency_in_status() {
    QFETCH(const int, frequency);
    QFETCH(const QString, text);

    auto &song_widget = song_editor.song_widget;

    song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
        frequency);
    QCOMPARE(get_model(song_widget.switch_column.switch_table)
                 .index(0, 0)
                 .data(Qt::StatusTipRole),
             text);
    song_widget.undo_stack.undo();
  };

  void test_gain() {
    auto &song_widget = song_editor.song_widget;
    auto &gain_editor = song_widget.controls_column.spin_boxes.gain_editor;

    const auto old_gain = get_gain(song_widget);
    QCOMPARE_NE(old_gain, NEW_GAIN_1);
    QCOMPARE_NE(old_gain, NEW_GAIN_2);

    gain_editor.setValue(NEW_GAIN_1);
    QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
    gain_editor.setValue(NEW_GAIN_2);
    QCOMPARE(get_gain(song_widget), NEW_GAIN_2);

    song_widget.undo_stack.undo();
    QCOMPARE(get_gain(song_widget), old_gain);
  };

  // regression test: FluidDriver's move-assignment operator must free any
  // audio driver it already owns before taking on a new one, and must be a
  // no-op on self-move-assignment -- the original bug overwrote
  // internal_pointer unconditionally, which would leak a live driver on
  // reassignment and, on self-move specifically, null out internal_pointer
  // without ever freeing it, losing the handle entirely
  static void test_fluid_driver_move_assign() {
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
  };

  static void test_insert_after_data() { add_tables(); };

  void test_insert_after() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();

    // voices can only be appended after the last row, not inserted in the
    // middle
    const auto is_voice =
        row_type == RowType::pitched_voice_type || row_type == RowType::unpitched_voice_type;
    select_cell(switch_table, is_voice ? old_row_count - 1 : 0, 0);
    song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action
        .trigger();

    QCOMPARE(model.rowCount(), old_row_count + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_insert_into_data() { add_tables(); };

  void test_insert_into() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();
    select_cell(switch_table, 0, 0);
    song_editor.song_menu_bar.edit_menu.insert_menu.insert_into_start_action
        .trigger();

    QCOMPARE(model.rowCount(), old_row_count + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_interval_button_data() {
    auto &controls_column = song_editor.song_widget.controls_column;
    auto &third_row = controls_column.third_row;
    auto &fifth_row = controls_column.fifth_row;
    auto &seventh_row = controls_column.seventh_row;
    auto &octave_row = controls_column.octave_row;
    auto &third_minus_button = third_row.minus_button;
    auto &third_plus_button = third_row.plus_button;
    auto &fifth_minus_button = fifth_row.minus_button;
    auto &fifth_plus_button = fifth_row.plus_button;
    auto &seventh_minus_button = seventh_row.minus_button;
    auto &seventh_plus_button = seventh_row.plus_button;
    auto &octave_minus_button = octave_row.minus_button;
    auto &octave_plus_button = octave_row.plus_button;

    QTest::addColumn<QPushButton *>("button_pointer");
    add_table_columns();

    QTest::newRow("chord third -") << &third_minus_button << RowType::chord_type << -1;
    QTest::newRow("chord third +") << &third_plus_button << RowType::chord_type << -1;
    QTest::newRow("chord fifth -") << &fifth_minus_button << RowType::chord_type << -1;
    QTest::newRow("chord fifth +") << &fifth_plus_button << RowType::chord_type << -1;
    QTest::newRow("chord seventh -")
        << &seventh_minus_button << RowType::chord_type << -1;
    QTest::newRow("chord seventh +")
        << &seventh_plus_button << RowType::chord_type << -1;
    QTest::newRow("chord octave -") << &octave_minus_button << RowType::chord_type << -1;
    QTest::newRow("chord octave +") << &octave_plus_button << RowType::chord_type << -1;
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
  };

  void test_interval_button() {
    QFETCH(QPushButton *, button_pointer);
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);
    const auto test_index =
        get_model(switch_table)
            .index(0, row_type == RowType::chord_type
                          ? static_cast<int>(ChordColumn::chord_interval_column)
                          : static_cast<int>(PitchedNoteColumn::pitched_note_interval_column));
    const auto original_data = test_index.data();
    get_selection_model(switch_table).select(test_index, SELECT_AND_CLEAR);

    get_reference(button_pointer).click();
    QCOMPARE_NE(original_data, test_index.data());
    undo_stack.undo();
    QCOMPARE(original_data, test_index.data());
    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_voice_error_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("error_message");

    static const QString header =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity>";
    static const QString one_pitched_voice =
        "<pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>";
    static const QString one_unpitched_voice =
        "<unpitched_voices><unpitched_voice><name>B</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices>";

    QTest::newRow("pitched voice number out of range")
        << header + one_pitched_voice + one_unpitched_voice +
               "<chords><chord><pitched_notes><pitched_note>"
               "<voice_number>5</voice_number></pitched_note></"
               "pitched_notes></chord></chords></song>"
        << "Voice 5 for chord 1, pitched note 1 has no corresponding voice";
    QTest::newRow("unpitched voice number out of range")
        << header + one_pitched_voice + one_unpitched_voice +
               "<chords><chord><unpitched_notes><unpitched_note>"
               "<voice_number>5</voice_number></unpitched_note></"
               "unpitched_notes></chord></chords></song>"
        << "Voice 5 for chord 1, unpitched note 1 has no corresponding voice";
    QTest::newRow("duplicate pitched voice name")
        << header +
               "<pitched_voices><pitched_voice><name>A</name>"
               "<instrument>Marimba</instrument></pitched_voice>"
               "<pitched_voice><name>A</name><instrument>Grand Piano</"
               "instrument></pitched_voice></pitched_voices>" +
               one_unpitched_voice + "</song>"
        << "Duplicate voice name \"A\"!";
    QTest::newRow("duplicate unpitched voice name")
        << header + one_pitched_voice +
               "<unpitched_voices><unpitched_voice><name>B</name>"
               "<percussion_set_pointer>Room</percussion_set_pointer>"
               "<midi_number>36</midi_number></unpitched_voice>"
               "<unpitched_voice><name>B</name><percussion_set_pointer>Power</"
               "percussion_set_pointer><midi_number>37</midi_number></"
               "unpitched_voice></unpitched_voices></song>"
        << "Duplicate voice name \"B\"!";
    QTest::newRow("empty pitched voice name")
        << header +
               "<pitched_voices><pitched_voice><name></name>"
               "<instrument>Marimba</instrument></pitched_voice></"
               "pitched_voices>" +
               one_unpitched_voice + "</song>"
        << "Voice name is empty!";
    QTest::newRow("empty unpitched voice name")
        << header + one_pitched_voice +
               "<unpitched_voices><unpitched_voice><name></name>"
               "<percussion_set_pointer>Room</percussion_set_pointer>"
               "<midi_number>36</midi_number></unpitched_voice></"
               "unpitched_voices></song>"
        << "Voice name is empty!";
  };

  void test_voice_error() {
    QFETCH(const QString, text);
    QFETCH(const QString, error_message);

    auto &song_widget = song_editor.song_widget;
    auto &chords_model = song_widget.switch_column.switch_table.chords_model;
    const auto old_current_file = song_widget.current_file;
    const auto old_chord_count = chords_model.rowCount(QModelIndex());
    QVERIFY(old_chord_count > 0);

    close_message_later(song_editor, waiting_for_message, error_message);
    open_text(song_widget, text);

    // a file that fails voice validation must leave the previously open
    // song untouched rather than clearing it out from under the user (open_file
    // used to clear/repopulate the models before validating, so a rejected
    // file silently wiped out whatever was open, with no undo path back)
    QCOMPARE(song_widget.current_file, old_current_file);
    QCOMPARE(chords_model.rowCount(QModelIndex()), old_chord_count);
  };

  static void test_voice_name_rejected_data() {
    add_table_columns();
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QVariant>("new_value");
    QTest::addColumn<QString>("warning_message");

    QTest::newRow("pitched voice duplicate name")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
        << QVariant(QString("Guitar")) << "Voice \"Guitar\" already exists!";
    QTest::newRow("pitched voice empty name")
        << RowType::pitched_voice_type << -1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column) << QVariant(QString())
        << "Voice name is empty!";
    QTest::newRow("unpitched voice duplicate name")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
        << QVariant(QString("Room Kit"))
        << "Voice \"Room Kit\" already exists!";
    QTest::newRow("unpitched voice empty name")
        << RowType::unpitched_voice_type << -1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column) << QVariant(QString())
        << "Voice name is empty!";
  };

  void test_voice_name_rejected() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, column_number);
    QFETCH(const QVariant, new_value);
    QFETCH(const QString, warning_message);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    const auto test_index = model.index(0, column_number);
    const auto old_value = test_index.data();

    close_message_later(song_editor, waiting_for_message, warning_message);
    QVERIFY(!model.setData(test_index, new_value, Qt::EditRole));
    QCOMPARE(test_index.data(), old_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_remove_voice_reassigns_notes_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("is_pitched");
    QTest::addColumn<QString>("warning_message");

    static const QString pitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice><pitched_voice>"
        "<name>B</name><instrument>Grand Piano</instrument></pitched_voice>"
        "<pitched_voice><name>C</name><instrument>Harp</instrument></"
        "pitched_voice></pitched_voices><unpitched_voices><unpitched_voice>"
        "<name>D</name><percussion_set_pointer>Room</percussion_set_pointer>"
        "<midi_number>36</midi_number></unpitched_voice></unpitched_voices>"
        "<chords><chord><pitched_notes><pitched_note><voice_number>0</"
        "voice_number></pitched_note><pitched_note><voice_number>1</"
        "voice_number></pitched_note><pitched_note><voice_number>2</"
        "voice_number></pitched_note></pitched_notes></chord></chords></"
        "song>";
    static const QString unpitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice><unpitched_voice><name>E</name>"
        "<percussion_set_pointer>Power</percussion_set_pointer><midi_number>37</"
        "midi_number></unpitched_voice><unpitched_voice><name>F</name>"
        "<percussion_set_pointer>Electronic</percussion_set_pointer>"
        "<midi_number>38</midi_number></unpitched_voice></unpitched_voices>"
        "<chords><chord><unpitched_notes><unpitched_note><voice_number>0</"
        "voice_number></unpitched_note><unpitched_note><voice_number>1</"
        "voice_number></unpitched_note><unpitched_note><voice_number>2</"
        "voice_number></unpitched_note></unpitched_notes></chord></chords></"
        "song>";

    QTest::newRow("pitched voice")
        << pitched_song << true
        << "Reassigning 1 pitched note voice to the first voice \"A\"";
    QTest::newRow("unpitched voice")
        << unpitched_song << false
        << "Reassigning 1 unpitched note voice to the first voice \"D\"";
  };

  void test_remove_voice_reassigns_notes() {
    // removing voice 1 of 3 should: leave notes on voice 0 alone, reassign
    // (and warn about the first) note on the removed voice 1 to voice 0, and
    // shift notes on voice 2 down to voice 1
    QFETCH(const QString, text);
    QFETCH(const bool, is_pitched);
    QFETCH(const QString, warning_message);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &song = song_widget.song;
    const auto voice_row_type = is_pitched ? RowType::pitched_voice_type
                                           : RowType::unpitched_voice_type;

    open_text(song_widget, text);

    switch_to(song_editor, voice_row_type, -1);
    QCOMPARE(get_model(switch_table).rowCount(), 3);
    select_cell(switch_table, 1, 0);
    close_message_later(song_editor, waiting_for_message, warning_message);
    song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();
    QCOMPARE(get_model(switch_table).rowCount(), 2);

    if (is_pitched) {
      const auto &notes = song.chords.at(0).pitched_notes;
      QCOMPARE(song.pitched_voices.size(), 2);
      QCOMPARE(notes.at(0).voice_number, 0);
      QCOMPARE(notes.at(1).voice_number, 0);
      QCOMPARE(notes.at(2).voice_number, 1);
    } else {
      const auto &notes = song.chords.at(0).unpitched_notes;
      QCOMPARE(song.unpitched_voices.size(), 2);
      QCOMPARE(notes.at(0).voice_number, 0);
      QCOMPARE(notes.at(1).voice_number, 0);
      QCOMPARE(notes.at(2).voice_number, 1);
    }

    undo_stack.undo(); // undo the voice removal

    if (is_pitched) {
      const auto &notes = song.chords.at(0).pitched_notes;
      QCOMPARE(song.pitched_voices.size(), 3);
      QCOMPARE(notes.at(0).voice_number, 0);
      QCOMPARE(notes.at(1).voice_number, 1);
      QCOMPARE(notes.at(2).voice_number, 2);
    } else {
      const auto &notes = song.chords.at(0).unpitched_notes;
      QCOMPARE(song.unpitched_voices.size(), 3);
      QCOMPARE(notes.at(0).voice_number, 0);
      QCOMPARE(notes.at(1).voice_number, 1);
      QCOMPARE(notes.at(2).voice_number, 2);
    }

    maybe_switch_back_to_chords(undo_stack, voice_row_type);

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  // regression test: RemoveVoiceRows::redo() used to shift note voice_numbers
  // and remove the voice rows only *after* showing the "reassigned" warning
  // dialog. QMessageBox::warning runs a nested event loop, so anything that
  // repainted while that dialog was up would see a voices list that hadn't
  // shrunk yet alongside notes already (or not yet) renumbered to match the
  // post-removal state -- a transient mismatch. This checks that by the time
  // the warning dialog appears, song.pitched_voices and every note's
  // voice_number already agree with each other.
  void test_remove_voice_row_consistent_during_warning() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &song = song_widget.song;

    open_text(
        song_widget,
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice><pitched_voice>"
        "<name>B</name><instrument>Grand Piano</instrument></pitched_voice>"
        "<pitched_voice><name>C</name><instrument>Harp</instrument></"
        "pitched_voice></pitched_voices><unpitched_voices><unpitched_voice>"
        "<name>D</name><percussion_set_pointer>Room</percussion_set_pointer>"
        "<midi_number>36</midi_number></unpitched_voice></unpitched_voices>"
        "<chords><chord><pitched_notes><pitched_note><voice_number>0</"
        "voice_number></pitched_note><pitched_note><voice_number>1</"
        "voice_number></pitched_note><pitched_note><voice_number>2</"
        "voice_number></pitched_note></pitched_notes></chord></chords></"
        "song>");

    switch_to(song_editor, RowType::pitched_voice_type, -1);
    select_cell(switch_table, 1, 0);

    const auto waiting_before = waiting_for_message;
    waiting_for_message = true;
    auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
        *(new QTimer(&song_editor));
    timer.setSingleShot(true);
    QObject::connect(
        &timer, &QTimer::timeout, &song_editor,
        [this, &song]() -> auto {
          auto *const box_pointer = find_top_level_message_box();
          if (box_pointer != nullptr) {
            waiting_for_message = false;
            QCOMPARE(song.pitched_voices.size(), 2);
            for (const auto &note : song.chords.at(0).pitched_notes) {
              QVERIFY(note.voice_number >= 0 &&
                      note.voice_number < song.pitched_voices.size());
            }
            QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
          }
        });
    timer.start(WAIT_TIME);
    QVERIFY(!waiting_before);

    song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();
    QVERIFY(!waiting_for_message);

    QCOMPARE(song.pitched_voices.size(), 2);
    const auto &notes = song.chords.at(0).pitched_notes;
    QCOMPARE(notes.at(0).voice_number, 0);
    QCOMPARE(notes.at(1).voice_number, 0);
    QCOMPARE(notes.at(2).voice_number, 1);

    undo_stack.undo(); // undo the voice removal
    maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  }

  static void test_remove_last_voice_error_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("is_pitched");
    QTest::addColumn<QString>("warning_message");

    static const QString song_with_one_of_each_voice =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>B</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices></song>";

    QTest::newRow("pitched voice")
        << song_with_one_of_each_voice << true
        << "Cannot remove every pitched voice; at least one must remain";
    QTest::newRow("unpitched voice")
        << song_with_one_of_each_voice << false
        << "Cannot remove every unpitched voice; at least one must remain";
  };

  void test_remove_last_voice_error() {
    // removing the only remaining voice of a type should warn and leave the
    // voice (and undo stack) untouched, rather than removing it
    QFETCH(const QString, text);
    QFETCH(const bool, is_pitched);
    QFETCH(const QString, warning_message);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    const auto voice_row_type = is_pitched ? RowType::pitched_voice_type
                                           : RowType::unpitched_voice_type;

    open_text(song_widget, text);

    switch_to(song_editor, voice_row_type, -1);
    auto &model = get_model(switch_table);
    QCOMPARE(model.rowCount(), 1);
    const auto old_undo_count = undo_stack.count();

    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message, warning_message);
    song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(undo_stack.count(), old_undo_count);

    maybe_switch_back_to_chords(undo_stack, voice_row_type);

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_unreduced_ratio_from_xml_data() {
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
  };

  // regression test: a chord's ratio/interval loaded from XML whose
  // numerator/denominator aren't already coprime (the schema only bounds
  // each to [1, 999], not that the pair is reduced) must be normalized to
  // the same canonical form every Rational/Interval constructor produces.
  // Otherwise the cell displays a value the UI can never actually produce
  // (e.g. "2/2" instead of blank), and re-committing the unchanged value
  // through the editor -- which always writes back a reduced value -- looks
  // like a real edit and pushes a spurious undo entry.
  void test_unreduced_ratio_from_xml() {
    QFETCH(const QString, text);
    QFETCH(const int, column_number);
    QFETCH(const QString, expected_text);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    open_text(song_widget, text);

    auto &model = get_model(switch_table);
    const auto test_index = model.index(0, column_number);

    QCOMPARE(test_index.data().toString(), expected_text);

    const auto old_undo_count = undo_stack.count();
    auto &delegate = get_reference(switch_table.itemDelegate());
    auto &cell_editor = get_reference(
        delegate.createEditor(&get_reference(switch_table.viewport()),
                              QStyleOptionViewItem(), test_index));
    delegate.setEditorData(&cell_editor, test_index);
    delegate.setModelData(&cell_editor, &model, test_index);

    QCOMPARE(undo_stack.count(), old_undo_count);

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  // regression test: Interval's folding constructor used to halve the
  // numerator while it was even without checking for 0 first -- 0 % 2 == 0
  // forever, so a 0 numerator hung the constructor instead of terminating.
  // Not reachable through the UI or schema-validated XML today (both bound
  // numerator to >= 1), but the constructor shouldn't hang on it regardless.
  static void test_interval_zero_numerator_does_not_hang() {
    Rational zero_ratio;
    zero_ratio.numerator = 0;
    zero_ratio.denominator = 1;

    const Interval interval(zero_ratio, 3);

    QCOMPARE(interval.ratio.numerator, 0);
    QCOMPARE(interval.ratio.denominator, 1);
    QCOMPARE(interval.octave, 3);
  };

  // regression test: insert_xml_rows used to always push_back the parsed
  // rows onto the end of the underlying list while announcing the insertion
  // at first_row_number via beginInsertRows/endInsertRows -- correct only
  // when first_row_number happens to equal the list's current size. Every
  // production call site loads into a freshly-cleared, empty model at row 0,
  // where append and position-0-insert coincide, so the bug was never
  // triggered in practice. Inserting a second batch ahead of already-loaded
  // rows exercises the general case directly against the model.
  static void test_insert_xml_rows_respects_first_row_number() {
    Song song;
    QUndoStack undo_stack;
    ChordsModel chords_model(undo_stack, song);
    chords_model.set_rows_pointer(&song.chords);

    const auto second_document = read_xml_document(
        "<chords><chord><words>second</words></chord></chords>");
    chords_model.insert_xml_rows(0, get_root(second_document));

    const auto first_document = read_xml_document(
        "<chords><chord><words>first</words></chord></chords>");
    chords_model.insert_xml_rows(0, get_root(first_document));

    QCOMPARE(chords_model.rowCount(QModelIndex()), 2);
    QCOMPARE(song.chords.at(0).words, QString("first"));
    QCOMPARE(song.chords.at(1).words, QString("second"));
  };

  static void test_musicxml_data() {
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<int>("number_of_chords");

    QTest::newRow("prelude") << "prelude.musicxml" << MUSIC_XML_ROWS;
    QTest::newRow("compressed prelude") << "prelude.mxl" << MUSIC_XML_ROWS;
    QTest::newRow("percussion") << "percussion.musicxml" << PERCUSSION_ROWS;
    QTest::newRow("transposing instruments")
        << "MozartTrio.musicxml" << MOZART_ROWS;
    QTest::newRow("repeats") << "Saltarello.musicxml" << SALTARELLO_ROWS;
  };

  void test_musicxml() {
    QFETCH(const QString, file_name);
    QFETCH(const int, number_of_chords);

    auto &song_widget = song_editor.song_widget;

    import_musicxml(song_widget, test_dir.filePath(file_name));
    QCOMPARE(get_model(song_widget.switch_column.switch_table)
                 .rowCount(QModelIndex()),
             number_of_chords);
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_musicxml_error_data() {
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<QString>("error_message");

    QTest::newRow("not musicxml")
        << "not_musicxml.xml" << "Invalid musicxml file";
    QTest::newRow("invalid mxl") << "invalid.mxl" << "Invalid XML file";
    QTest::newRow("empty") << "empty.musicxml" << "No chords";
    QTest::newRow("grace notes") << "MozartPianoSonata.musicxml"
                                 << "Notes without durations not supported";
    QTest::newRow("timewise")
        << "timewise.musicxml"
        << "Justly only supports partwise musicxml scores";
    // regression test: divisions is xs:decimal in the musicxml schema (to
    // allow fractional divisions), but xml_to_int used to silently truncate
    // fractional content via std::stoi instead of rejecting it, corrupting
    // note timing with no warning
    QTest::newRow("fractional divisions")
        << "fractional_divisions.musicxml"
        << "Fractional divisions are not supported";
  };

  void test_musicxml_error() {
    QFETCH(const QString, error_message);
    QFETCH(const QString, file_name);

    close_message_later(song_editor, waiting_for_message, error_message);
    import_musicxml(song_editor.song_widget, test_dir.filePath(file_name));
  };

  // regression test: a backward repeat with no forward repeat since the
  // last block boundary (e.g. a repeat that implicitly continues right
  // after an earlier, already-expanded repeated section) must replay from
  // that block boundary, not from measure 0 -- otherwise an earlier,
  // already-consumed repeated section gets incorrectly replayed again, and
  // the plain measures between the two sections get silently dropped
  // instead of flushed
  static void test_compute_measure_expansion_lone_backward_repeat() {
    QList<MeasureRepeatInfo> measure_infos;

    MeasureRepeatInfo measure_0;
    measure_0.start_time = 0;
    measure_0.end_time = 10;
    measure_0.has_forward_repeat = true;
    measure_infos.push_back(measure_0);

    MeasureRepeatInfo measure_1;
    measure_1.start_time = 10;
    measure_1.end_time = 20;
    measure_1.has_backward_repeat = true;
    measure_infos.push_back(measure_1);

    MeasureRepeatInfo measure_2;
    measure_2.start_time = 20;
    measure_2.end_time = 30;
    measure_infos.push_back(measure_2);

    MeasureRepeatInfo measure_3;
    measure_3.start_time = 30;
    measure_3.end_time = 40;
    measure_infos.push_back(measure_3);

    // a lone backward repeat: no forward repeat since measure 2
    MeasureRepeatInfo measure_4;
    measure_4.start_time = 40;
    measure_4.end_time = 50;
    measure_4.has_backward_repeat = true;
    measure_infos.push_back(measure_4);

    const QList<std::pair<int, int>> expected_expansion = {
        {0, 10}, {10, 20}, {0, 10}, {10, 20},  // measures 0-1, twice
        {20, 30}, {30, 40}, {40, 50},          // measures 2-4, ...
        {20, 30}, {30, 40}, {40, 50},          // ...twice
    };

    QCOMPARE(compute_measure_expansion(measure_infos), expected_expansion);
  };

  // regression test: the musicxml importer's in-progress-tie lookup used to
  // be keyed by pitch alone, shared across every voice (and every part) in
  // the whole document. Two simultaneous voices tying the same pitch (here,
  // two instruments in one piano part) would clobber each other's still-open
  // note: the second voice's tie-start silently overwrote the first voice's
  // map entry, so the first voice's tie-stop resolved onto the wrong note
  // (wrong words/duration) and the second voice's own tie-stop then found no
  // entry at all. Keying the lookup by voice as well as pitch keeps
  // overlapping ties on the same pitch independent.
  void test_import_musicxml_ties_do_not_cross_voices() {
    auto &song_widget = song_editor.song_widget;

    import_musicxml(song_widget, test_dir.filePath("tied_voices.musicxml"));

    auto &song = song_widget.song;
    QCOMPARE(song.chords.size(), 2);

    const auto &left_hand_notes = song.chords.at(0).pitched_notes;
    QCOMPARE(left_hand_notes.size(), 1);
    QCOMPARE(left_hand_notes.at(0).voice_number, 0);
    QVERIFY(left_hand_notes.at(0).words.contains("Left Hand"));
    QCOMPARE(left_hand_notes.at(0).beats.numerator, 8);
    QCOMPARE(left_hand_notes.at(0).beats.denominator, 1);

    const auto &right_hand_notes = song.chords.at(1).pitched_notes;
    QCOMPARE(right_hand_notes.size(), 1);
    QCOMPARE(right_hand_notes.at(0).voice_number, 1);
    QVERIFY(right_hand_notes.at(0).words.contains("Right Hand"));
    QCOMPARE(right_hand_notes.at(0).beats.numerator, 8);
    QCOMPARE(right_hand_notes.at(0).beats.denominator, 1);

    open_file(song_widget, test_dir.filePath("test_song.xml"));
  };

  // regression test: a <tie type="stop"/> with no matching earlier
  // <tie type="start"/> for the same pitch/voice used to dereference a
  // missing map entry, guarded only by a release-mode-noop Q_ASSERT -- a
  // malformed or hand-edited musicxml file could trigger undefined behavior.
  // It must now import as an ordinary, unstarted note instead.
  void test_import_musicxml_orphan_tie_stop() {
    auto &song_widget = song_editor.song_widget;

    import_musicxml(song_widget, test_dir.filePath("orphan_tie.musicxml"));

    auto &song = song_widget.song;
    QCOMPARE(song.chords.size(), 1);

    const auto &notes = song.chords.at(0).pitched_notes;
    QCOMPARE(notes.size(), 1);
    QCOMPARE(notes.at(0).beats.numerator, 4);
    QCOMPARE(notes.at(0).beats.denominator, 1);

    open_file(song_widget, test_dir.filePath("test_song.xml"));
  };

  // regression test: read_zip_entry casts a zip entry's reported size down
  // to int before allocating its buffer, but reads however many bytes the
  // (uncast, 64-bit) size claims -- an entry whose declared size doesn't fit
  // in an int, or whose size libzip couldn't report at all, must be rejected
  // up front instead of under-allocating the destination buffer
  static void test_zip_entry_size_is_safe_data() {
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
  };

  static void test_zip_entry_size_is_safe() {
    QFETCH(const unsigned int, valid_flags);
    QFETCH(const zip_uint64_t, size);
    QFETCH(const bool, is_safe);

    zip_stat_t entry_stat;
    zip_stat_init(&entry_stat);
    entry_stat.valid = valid_flags;
    entry_stat.size = size;

    QCOMPARE(zip_entry_size_is_safe(entry_stat), is_safe);
  };

  // regression test: read_zip_entry used to only Q_ASSERT that
  // internal_pointer wasn't null before dereferencing it -- an assert that
  // compiles away in release builds. A ZipArchive constructed from a file
  // that doesn't exist (or isn't a zip) leaves internal_pointer null, and
  // read_zip_entry must degrade to its documented "empty QByteArray" return
  // instead of crashing.
  void test_read_zip_entry_null_archive() const {
    const ZipArchive archive(test_dir.filePath("does_not_exist.zip"));
    QCOMPARE(archive.internal_pointer, nullptr);
    QCOMPARE(read_zip_entry(archive, "anything"), QByteArray());
  };

  // regression test: read_xml_document casts a QByteArray's size down to int
  // before handing it to xmlReadMemory, but xmlReadMemory reads however many
  // bytes the (uncast) length claims -- a buffer whose size doesn't fit in an
  // int must be rejected up front instead of under-reporting its length
  static void test_xml_bytes_size_is_safe_data() {
    QTest::addColumn<qsizetype>("size");
    QTest::addColumn<bool>("is_safe");

    QTest::newRow("ordinary small buffer") << qsizetype{13} << true;
    QTest::newRow("largest int-sized buffer")
        << static_cast<qsizetype>(std::numeric_limits<int>::max()) << true;
    QTest::newRow("just over int-sized buffer")
        << static_cast<qsizetype>(std::numeric_limits<int>::max()) + 1
        << false;
  };

  static void test_xml_bytes_size_is_safe() {
    QFETCH(const qsizetype, size);
    QFETCH(const bool, is_safe);

    QCOMPARE(xml_bytes_size_is_safe(size), is_safe);
  };

  // regression test: some musicxml fields (e.g. fifths, octave-change) are
  // unbounded xs:integer with no schema-enforced range, so std::stoi can
  // throw std::out_of_range on a magnitude that doesn't fit in int; xml_to_int
  // must clamp instead of letting that exception propagate uncaught and
  // crash the app
  static void test_xml_to_int_clamps_out_of_range_data() {
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<int>("expected");

    QTest::newRow("ordinary value") << QByteArray("<n>42</n>") << 42;
    QTest::newRow("overflow positive")
        << QByteArray("<n>99999999999999999999</n>")
        << std::numeric_limits<int>::max();
    QTest::newRow("overflow negative")
        << QByteArray("<n>-99999999999999999999</n>")
        << std::numeric_limits<int>::min();
  };

  static void test_xml_to_int_clamps_out_of_range() {
    QFETCH(const QByteArray, xml);
    QFETCH(const int, expected);

    const auto document = read_xml_document(xml);
    QCOMPARE(xml_to_int(get_root(document)), expected);
  };

  // get_share_file's missing-file path (Q_ASSERT compiles out in release
  // builds, so a broken/incomplete installation missing a bundled resource
  // -- an xsd schema, the icon, the soundfont -- must still be rejected
  // regardless of build type) now shows QMessageBox::critical and calls
  // std::exit(), so it can't be exercised from within this test binary
  // without killing the whole run; not covered here.
  void test_get_share_file_existing() const {
    QCOMPARE(get_share_file("Justly.svg"),
             test_dir.filePath("Justly.svg").toStdString());
  };

  // regression test: inserting a chord, then drilling into and inserting one
  // of its notes, leaves the switch table's notes model pointing directly at
  // that Chord's notes QList; import_musicxml/open_file must not crash even
  // though they replace song.chords wholesale while that pointer is live
  void test_import_musicxml_after_editing_chord_notes() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &insert_menu = song_editor.song_menu_bar.edit_menu.insert_menu;

    select_cell(switch_table, 0, 0);
    insert_menu.insert_after_action.trigger();
    switch_to(song_editor, RowType::pitched_note_type, 1);
    insert_menu.insert_into_start_action.trigger();

    import_musicxml(song_widget, test_dir.filePath("prelude.musicxml"));
    QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), MUSIC_XML_ROWS);

    open_file(song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_next_previous_data() {
    add_table_columns();

    QTest::newRow("pitched note") << RowType::pitched_note_type << 1;
    QTest::newRow("unpitched note") << RowType::unpitched_note_type << 1;
  };

  void test_next_previous() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &view_menu = song_editor.song_menu_bar.view_menu;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_parent_chord_number(switch_table), chord_number);
    view_menu.previous_chord_action.trigger();
    QCOMPARE(get_parent_chord_number(switch_table), chord_number - 1);
    view_menu.next_chord_action.trigger();
    QCOMPARE(get_parent_chord_number(switch_table), chord_number);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  // replace_table() (ReplaceTable.hpp) reconnects an update lambda to the
  // switch table's selection model on every call, but only swaps in a fresh
  // selection model (via set_model) when the row type actually changes --
  // when it doesn't (e.g. next/previous_chord_action, exercised above),
  // the same QItemSelectionModel is reused across calls, so a bare
  // connect() without a preceding disconnect() piles up one duplicate
  // connection per call. This reproduces that exact connect/reconnect
  // pattern against the real selection model object used by the switch
  // table, using a private receiver so it can't disturb replace_table's own
  // connection to that same model.
  void test_reconnecting_selection_model_does_not_duplicate_connections() {
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &selection_model = get_selection_model(switch_table);

    // control: connecting without disconnecting first (the pre-fix pattern)
    // really does accumulate one call per reconnect
    {
      select_cell(switch_table, 0, 1);
      QObject receiver;
      auto call_count = 0;
      for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
        QObject::connect(&selection_model,
                         &QItemSelectionModel::selectionChanged, &receiver,
                         [&call_count]() -> auto { call_count = call_count + 1; });
      }
      select_cell(switch_table, 0, 0);
      QCOMPARE(call_count, 5);
    }

    // the fix: disconnecting before reconnecting keeps exactly one
    // connection alive no matter how many times replace_table() re-runs
    // against the same selection model
    {
      select_cell(switch_table, 0, 1);
      QObject receiver;
      auto call_count = 0;
      for (auto attempt = 0; attempt < 5; attempt = attempt + 1) {
        QObject::disconnect(&selection_model,
                            &QItemSelectionModel::selectionChanged, &receiver,
                            nullptr);
        QObject::connect(&selection_model,
                         &QItemSelectionModel::selectionChanged, &receiver,
                         [&call_count]() -> auto { call_count = call_count + 1; });
      }
      select_cell(switch_table, 0, 0);
      QCOMPARE(call_count, 1);
    }
  };

  void test_octave_bound() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, RowType::pitched_note_type, 1);
    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message,
                        "Octave 10 (absolutely) greater than maximum 9");
    press_times(song_widget.controls_column.octave_row.plus_button,
                OCTAVE_SHIFT_TIMES + 1);
    undo_times(undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave

    maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
  };

  static void test_open_error_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("error_message");

    QTest::newRow("not xml") << "<" << "Invalid XML file";
    QTest::newRow("not Justly") << "<song/>" << "Invalid song file";
  };

  void test_open_error() {
    QFETCH(const QString, text);
    QFETCH(const QString, error_message);

    auto &song_widget = song_editor.song_widget;
    close_message_later(song_editor, waiting_for_message, error_message);
    open_text(song_widget, text);
  };

  static void test_paste_after_data() { add_cells(); };

  void test_paste_after() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, row_number);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    select_cell(switch_table, row_number, column_number);
    edit_menu.copy_action.trigger();

    const auto number_of_rows = model.rowCount();
    edit_menu.paste_menu.paste_after_action.trigger();
    QCOMPARE(model.rowCount(), number_of_rows + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), number_of_rows);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_paste_error_data() {
    add_table_columns();
    QTest::addColumn<QString>("copied");
    QTest::addColumn<QString>("mime_type");
    QTest::addColumn<QString>("error_message");
    QTest::newRow("chord not a mime")
        << RowType::chord_type << -1 << "" << "not a mime"
        << "Cannot paste not a mime as chords cells";
    QTest::newRow("chord pitched notes mime")
        << RowType::chord_type << -1 << "" << PitchedNote::get_cells_mime()
        << "Cannot paste pitched notes cells as chords cells";
    QTest::newRow("chord not xml")
        << RowType::chord_type << -1 << "[" << Chord::get_cells_mime() << "Invalid XML";
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
        << RowType::pitched_note_type << 1 << "<song/>" << PitchedNote::get_cells_mime()
        << "Invalid clipboard";
    QTest::newRow("unpitched note not a mime")
        << RowType::unpitched_note_type << 1 << "" << "not a mime"
        << "Cannot paste not a mime as unpitched notes cells";
    QTest::newRow("unpitched note chords mime")
        << RowType::unpitched_note_type << 1 << "" << Chord::get_cells_mime()
        << "Cannot paste chords cells as unpitched notes cells";
    QTest::newRow("unpitched note not xml")
        << RowType::unpitched_note_type << 1 << "<" << UnpitchedNote::get_cells_mime()
        << "Invalid XML";
    QTest::newRow("unpitched note not Justly")
        << RowType::unpitched_note_type << 1 << "<song/>"
        << UnpitchedNote::get_cells_mime() << "Invalid clipboard";
  };

  void test_paste_error() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const QString, copied);
    QFETCH(const QString, mime_type);
    QFETCH(const QString, error_message);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &new_data =
        get_reference(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)

    new_data.setData(mime_type, copied.toStdString().c_str());

    auto &clipboard = get_reference(QGuiApplication::clipboard());
    clipboard.setMimeData(&new_data);

    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message, error_message);
    song_editor.song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_paste_into_data() { add_cells(); };

  void test_paste_into() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, row_number);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);

    select_cell(switch_table, row_number, column_number);
    edit_menu.copy_action.trigger();

    const auto number_of_rows = model.rowCount();
    edit_menu.paste_menu.paste_into_start_action.trigger();
    QCOMPARE(model.rowCount(), number_of_rows + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), number_of_rows);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_paste_stale_voice_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("is_pitched");

    static const QString pitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice><pitched_voice>"
        "<name>B</name><instrument>Grand Piano</instrument></pitched_voice>"
        "</pitched_voices><unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<pitched_notes><pitched_note><voice_number>0</voice_number>"
        "</pitched_note><pitched_note><voice_number>1</voice_number>"
        "</pitched_note></pitched_notes></chord></chords></song>";
    static const QString unpitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice><unpitched_voice><name>E</name>"
        "<percussion_set_pointer>Power</percussion_set_pointer><midi_number>37</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<unpitched_notes><unpitched_note><voice_number>0</voice_number>"
        "</unpitched_note><unpitched_note><voice_number>1</voice_number>"
        "</unpitched_note></unpitched_notes></chord></chords></song>";

    QTest::newRow("pitched voice") << pitched_song << true;
    QTest::newRow("unpitched voice") << unpitched_song << false;
  };

  void test_paste_stale_voice() {
    // if the clipboard holds a note referencing a voice, and that voice gets
    // deleted before the paste happens, the clipboard's voice_number must be
    // reassigned to the first remaining voice the same way the live note is,
    // so pasting lands on a valid voice instead of indexing into the voices
    // list with the now out-of-range voice number
    QFETCH(const QString, text);
    QFETCH(const bool, is_pitched);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &back_to_chords_action =
        song_editor.song_menu_bar.view_menu.back_to_chords_action;
    auto &song = song_widget.song;

    const auto note_row_type =
        is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
    const auto voice_row_type = is_pitched ? RowType::pitched_voice_type
                                           : RowType::unpitched_voice_type;
    const auto voice_column =
        is_pitched
            ? static_cast<int>(
                  PitchedNoteColumn::pitched_note_voice_number_column)
            : static_cast<int>(
                  UnpitchedNoteColumn::unpitched_note_voice_number_column);
    const QList<QString> reassign_warnings =
        is_pitched
            ? QList<QString>{"Reassigning 1 pitched note voice to the "
                             "first voice \"A\"",
                             "Reassigning 1 clipboard pitched note voice to "
                             "the first voice \"A\""}
            : QList<QString>{"Reassigning 1 unpitched note voice to the "
                             "first voice \"D\"",
                             "Reassigning 1 clipboard unpitched note voice "
                             "to the first voice \"D\""};

    open_text(song_widget, text);

    // copy the second note's voice cell, which references the last (soon to
    // be removed) voice
    switch_to(song_editor, note_row_type, 0);
    select_cell(switch_table, 1, voice_column);
    edit_menu.copy_action.trigger();
    back_to_chords_action.trigger();

    // remove that voice; the fixture's only note gets reassigned to voice 0,
    // warning about that live note, and separately about the clipboard copy
    // (of that same note's voice cell) that also collapses to voice 0
    switch_to(song_editor, voice_row_type, -1);
    select_cell(switch_table, 1, 0);
    close_messages_later(song_editor, waiting_for_message, reassign_warnings);
    edit_menu.remove_rows_action.trigger();
    back_to_chords_action.trigger();

    // pasting the reassigned clipboard should succeed silently, landing on
    // the first remaining voice instead of erroring or misassigning
    switch_to(song_editor, note_row_type, 0);
    select_cell(switch_table, 0, voice_column);
    edit_menu.paste_menu.paste_over_action.trigger();
    QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                        : song.chords.at(0).unpitched_notes.at(0).voice_number,
             0);
    back_to_chords_action.trigger();

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_paste_voice_renumbered_on_insert_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("is_pitched");

    static const QString pitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice><pitched_voice>"
        "<name>B</name><instrument>Grand Piano</instrument></pitched_voice>"
        "</pitched_voices><unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<pitched_notes><pitched_note><voice_number>0</voice_number>"
        "</pitched_note><pitched_note><voice_number>1</voice_number>"
        "</pitched_note></pitched_notes></chord></chords></song>";
    static const QString unpitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice><unpitched_voice><name>E</name>"
        "<percussion_set_pointer>Power</percussion_set_pointer><midi_number>37</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<unpitched_notes><unpitched_note><voice_number>0</voice_number>"
        "</unpitched_note><unpitched_note><voice_number>1</voice_number>"
        "</unpitched_note></unpitched_notes></chord></chords></song>";

    QTest::newRow("pitched voice") << pitched_song << true;
    QTest::newRow("unpitched voice") << unpitched_song << false;
  };

  void test_paste_voice_renumbered_on_insert() {
    // if the clipboard holds a note referencing a voice, and a voice is
    // inserted before it, the clipboard's voice_number must shift the same
    // way the live note's does, or pasting would silently land on whatever
    // voice now occupies the old index instead of the voice actually copied
    QFETCH(const QString, text);
    QFETCH(const bool, is_pitched);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &back_to_chords_action =
        song_editor.song_menu_bar.view_menu.back_to_chords_action;
    auto &song = song_widget.song;

    const auto note_row_type =
        is_pitched ? RowType::pitched_note_type : RowType::unpitched_note_type;
    const auto voice_row_type = is_pitched ? RowType::pitched_voice_type
                                           : RowType::unpitched_voice_type;
    const auto voice_column =
        is_pitched
            ? static_cast<int>(
                  PitchedNoteColumn::pitched_note_voice_number_column)
            : static_cast<int>(
                  UnpitchedNoteColumn::unpitched_note_voice_number_column);

    open_text(song_widget, text);

    // copy the second note's voice cell, which references the second voice
    switch_to(song_editor, note_row_type, 0);
    select_cell(switch_table, 1, voice_column);
    edit_menu.copy_action.trigger();
    back_to_chords_action.trigger();

    // insert a new voice before both existing voices; the fixture's notes
    // both shift up by one, and so must the clipboard's copied voice_number
    switch_to(song_editor, voice_row_type, -1);
    select_cell(switch_table, 0, 0);
    edit_menu.insert_menu.insert_into_start_action.trigger();
    back_to_chords_action.trigger();

    // pasting the shifted clipboard should land on the second voice's new
    // index, not the stale pre-insert index (which now points elsewhere)
    switch_to(song_editor, note_row_type, 0);
    select_cell(switch_table, 0, voice_column);
    edit_menu.paste_menu.paste_over_action.trigger();
    QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                        : song.chords.at(0).unpitched_notes.at(0).voice_number,
             2);
    back_to_chords_action.trigger();

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_paste_chord_voice_renumbered_on_insert_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("is_pitched");

    static const QString pitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice><pitched_voice>"
        "<name>B</name><instrument>Grand Piano</instrument></pitched_voice>"
        "</pitched_voices><unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<pitched_notes><pitched_note><voice_number>0</voice_number>"
        "</pitched_note><pitched_note><voice_number>1</voice_number>"
        "</pitched_note></pitched_notes></chord></chords></song>";
    static const QString unpitched_song =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice><unpitched_voice><name>E</name>"
        "<percussion_set_pointer>Power</percussion_set_pointer><midi_number>37</"
        "midi_number></unpitched_voice></unpitched_voices><chords><chord>"
        "<unpitched_notes><unpitched_note><voice_number>0</voice_number>"
        "</unpitched_note><unpitched_note><voice_number>1</voice_number>"
        "</unpitched_note></unpitched_notes></chord></chords></song>";

    QTest::newRow("pitched voice") << pitched_song << true;
    QTest::newRow("unpitched voice") << unpitched_song << false;
  };

  void test_paste_chord_voice_renumbered_on_insert() {
    // copying a whole chord bakes its nested notes' voice_number into the
    // clipboard too; inserting a voice must shift those nested voice_numbers
    // the same way it shifts a flat note copy, or pasting the chord back
    // would silently restore the notes' stale, pre-insert voice numbers
    // (regression test for renumber_clipboard_voice_numbers not looking
    // inside a copied chord's nested pitched_notes/unpitched_notes)
    QFETCH(const QString, text);
    QFETCH(const bool, is_pitched);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &back_to_chords_action =
        song_editor.song_menu_bar.view_menu.back_to_chords_action;
    auto &song = song_widget.song;

    const auto voice_row_type = is_pitched ? RowType::pitched_voice_type
                                           : RowType::unpitched_voice_type;
    const auto last_column = Chord::get_number_of_columns() - 1;

    open_text(song_widget, text);

    // copy the whole chord row, including its pitched_notes/unpitched_notes
    // column, which nests both notes' voice_number fields in the clipboard
    get_selection_model(switch_table)
        .select(QItemSelection(get_model(switch_table).index(0, 0),
                               get_model(switch_table).index(0, last_column)),
                SELECT_AND_CLEAR);
    edit_menu.copy_action.trigger();

    // insert a new voice before both existing voices; the live chord's notes
    // shift up by one, and so must the nested voice_numbers on the clipboard
    switch_to(song_editor, voice_row_type, -1);
    select_cell(switch_table, 0, 0);
    edit_menu.insert_menu.insert_into_start_action.trigger();
    back_to_chords_action.trigger();

    // pasting the shifted clipboard back over the chord should restore both
    // notes at their new, shifted voice numbers, not the stale pre-insert ones
    get_selection_model(switch_table)
        .select(QItemSelection(get_model(switch_table).index(0, 0),
                               get_model(switch_table).index(0, last_column)),
                SELECT_AND_CLEAR);
    edit_menu.paste_menu.paste_over_action.trigger();

    QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(0).voice_number
                        : song.chords.at(0).unpitched_notes.at(0).voice_number,
             1);
    QCOMPARE(is_pitched ? song.chords.at(0).pitched_notes.at(1).voice_number
                        : song.chords.at(0).unpitched_notes.at(1).voice_number,
             2);

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_play_data() {

    add_table_columns();
    QTest::addColumn<int>("first_row_number");
    QTest::addColumn<int>("second_row_number");
    QTest::addColumn<int>("column_number");

    QTest::newRow("two chords") << RowType::chord_type << -1 << 0 << 1
                                << static_cast<int>(ChordColumn::chord_interval_column);
    QTest::newRow("one chord") << RowType::chord_type << -1 << 1 << 1
                               << static_cast<int>(ChordColumn::chord_interval_column);
    QTest::newRow("two pitched notes")
        << RowType::pitched_note_type << 1 << 0 << 1
        << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
    QTest::newRow("one pitched note")
        << RowType::pitched_note_type << 1 << 1 << 1
        << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
    QTest::newRow("two unpitched notes")
        << RowType::unpitched_note_type << 1 << 0 << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column);
    QTest::newRow("one unpitched note")
        << RowType::unpitched_note_type << 1 << 1 << 1
        << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column);
    QTest::newRow("two pitched voices")
        << RowType::pitched_voice_type << -1 << 0 << 1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
    QTest::newRow("one pitched voice")
        << RowType::pitched_voice_type << -1 << 1 << 1
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
    QTest::newRow("two unpitched voices")
        << RowType::unpitched_voice_type << -1 << 0 << 1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
    QTest::newRow("one unpitched voice")
        << RowType::unpitched_voice_type << -1 << 1 << 1
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_midi_number_column);
  };

  void test_play() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, first_row_number);
    QFETCH(const int, second_row_number);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &play_menu = song_editor.song_menu_bar.play_menu;
    auto &play_action = play_menu.play_action;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    get_selection_model(switch_table)
        .select(QItemSelection(model.index(first_row_number, column_number),
                               model.index(second_row_number, column_number)),
                SELECT_AND_CLEAR);
    play_action.trigger();
    // first cut off early
    play_action.trigger();
    // now play for a while
    QThread::msleep(WAIT_TIME);
    play_menu.stop_playing_action.trigger();

    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  void test_play_to_end_starts_playhead() {
    // regression test: "Play to end" is a separate action from "Play
    // selection" and must independently start the piano roll playhead
    // animation, not just trigger audio playback
    auto &song_widget = song_editor.song_widget;
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &play_menu = song_editor.song_menu_bar.play_menu;

    select_cell(switch_table, 0, 0);

    QVERIFY(!piano_roll_widget.piano_roll_view.playhead_active);
    play_menu.play_to_end_action.trigger();
    QVERIFY(piano_roll_widget.piano_roll_view.playhead_active);

    QThread::msleep(WAIT_TIME);
    play_menu.stop_playing_action.trigger();
    QVERIFY(!piano_roll_widget.piano_roll_view.playhead_active);
  };

  void test_ratio_bound_data() {
    QTest::addColumn<QPushButton *>("fifth_button_pointer");
    QTest::addColumn<QPushButton *>("octave_button_pointer");
    QTest::addColumn<QString>("error_message");

    auto &controls_column = song_editor.song_widget.controls_column;
    auto &fifth_row = controls_column.fifth_row;
    auto &octave_row = controls_column.octave_row;

    QTest::newRow("too high")
        << &fifth_row.plus_button << &octave_row.plus_button
        << "Numerator 2187 greater than maximum 999";

    QTest::newRow("too low")
        << &fifth_row.minus_button << &octave_row.minus_button
        << "Denominator 2187 greater than maximum 999";
  }

  void test_ratio_bound() {
    QFETCH(QPushButton *, fifth_button_pointer);
    QFETCH(QPushButton *, octave_button_pointer);
    QFETCH(const QString, error_message);

    auto &fifth_button = get_reference(fifth_button_pointer);
    auto &octave_button = get_reference(octave_button_pointer);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, RowType::pitched_note_type, 1);
    select_cell(switch_table, 0, 0);

    for (auto counter = 0; counter < RATIO_SHIFT_TIMES; counter++) {
      fifth_button.click();
      octave_button.click();
    }
    close_message_later(song_editor, waiting_for_message, error_message);
    fifth_button.click();
    undo_times(undo_stack, RATIO_SHIFT_TIMES * 2); // undo shift numerator

    maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
  };

  static void test_remove_row_data() { add_tables(); };

  void test_remove_row() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();

    select_cell(switch_table, 0, 0);
    // removing voice 0 reassigns the fixture's notes that use it, warning
    // about how many were reassigned
    if (row_type == RowType::pitched_voice_type) {
      close_message_later(
          song_editor, waiting_for_message,
          "Reassigning 7 pitched note voices to the first voice \"Guitar\"");
    } else if (row_type == RowType::unpitched_voice_type) {
      close_message_later(
          song_editor, waiting_for_message,
          "Reassigning 2 unpitched note voices to the first voice \"Room "
          "Kit\"");
    }
    song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();

    QCOMPARE(model.rowCount(), old_row_count - 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    // undoing a row removal should select the restored row across every
    // column, not leave the selection empty
    const auto &restored_range = get_only_range(switch_table);
    QCOMPARE(restored_range.top(), 0);
    QCOMPARE(restored_range.bottom(), 0);
    QCOMPARE(restored_range.left(), 0);
    QCOMPARE(restored_range.right(), model.columnCount() - 1);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_replace_table_combining() {
    auto &song_widget = song_editor.song_widget;
    switch_to(song_editor, RowType::unpitched_note_type, 0);
    song_editor.song_menu_bar.view_menu.back_to_chords_action.trigger();
    QVERIFY(!song_widget.undo_stack.canUndo());
  };

  static void test_row_count_data() {
    add_table_columns();
    QTest::addColumn<int>("number_of_rows");

    QTest::newRow("chords") << RowType::chord_type << -1 << EIGHT;
    QTest::newRow("chord 0 pitched notes") << RowType::pitched_note_type << 0 << 0;
    QTest::newRow("chord 1 pitched notes") << RowType::pitched_note_type << 1 << EIGHT;
    QTest::newRow("chord 2 pitched notes") << RowType::pitched_note_type << 2 << 0;
    QTest::newRow("chord 3 pitched notes") << RowType::pitched_note_type << 3 << 0;
    QTest::newRow("chord 4 pitched notes") << RowType::pitched_note_type << 4 << 0;
    QTest::newRow("chord 5 pitched notes") << RowType::pitched_note_type << FIVE << 0;
    QTest::newRow("chord 6 pitched notes") << RowType::pitched_note_type << SIX << 0;
    QTest::newRow("chord 7 pitched notes") << RowType::pitched_note_type << SEVEN << 0;
    QTest::newRow("chord 0 unpitched notes") << RowType::unpitched_note_type << 0 << 0;
    QTest::newRow("chord 1 unpitched notes") << RowType::unpitched_note_type << 1 << 4;
    QTest::newRow("chord 2 unpitched notes") << RowType::unpitched_note_type << 2 << 0;
    QTest::newRow("chord 3 unpitched notes") << RowType::unpitched_note_type << 3 << 0;
    QTest::newRow("chord 4 unpitched notes") << RowType::unpitched_note_type << 4 << 0;
    QTest::newRow("chord 5 unpitched notes")
        << RowType::unpitched_note_type << FIVE << 0;
    QTest::newRow("chord 6 unpitched notes") << RowType::unpitched_note_type << SIX << 0;
    QTest::newRow("chord 7 unpitched notes")
        << RowType::unpitched_note_type << SEVEN << 0;
    QTest::newRow("pitched voices") << RowType::pitched_voice_type << -1 << 2;
    QTest::newRow("unpitched voices") << RowType::unpitched_voice_type << -1 << 3;
  };

  void test_row_count() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, number_of_rows);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_model(switch_table).rowCount(), number_of_rows);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_row_header_data() {
    QTest::addColumn<Qt::ItemDataRole>("role");
    QTest::addColumn<QVariant>("data");

    QTest::newRow("text") << Qt::DisplayRole << QVariant(1);
    QTest::newRow("unused role") << Qt::DecorationRole << QVariant();
  };

  void test_row_header() {
    QFETCH(const Qt::ItemDataRole, role);
    QFETCH(const QVariant, data);

    QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
                 .headerData(0, Qt::Vertical, role),
             data);
  };

  void test_save() {
    auto &song_widget = song_editor.song_widget;
    auto &song_menu_bar = song_editor.song_menu_bar;

    auto original_text = get_file_text(test_dir.filePath("test_song.xml"));

    // save into a temp file
    auto save_filename = test_dir.filePath("test_song_2.xml");
    save_as_file(song_widget, save_filename);
    QCOMPARE(song_widget.current_file, save_filename);

    // compare the saved text to the original text
    QCOMPARE(original_text, get_file_text(save_filename));

    // now change the song and save to the same file
    song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
    song_menu_bar.file_menu.save_action.trigger();
    song_widget.undo_stack.undo();

    QCOMPARE_NE(original_text, get_file_text(save_filename));

    QFile(save_filename).remove();
  };

  void test_save_error_does_not_lose_work() {
    auto &song_widget = song_editor.song_widget;
    auto &undo_stack = song_widget.undo_stack;

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

    close_message_later(song_editor, waiting_for_message,
                        "Failed to save file");
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
  };

  void test_recovery_removed_on_save_and_open() {
    auto &song_widget = song_editor.song_widget;
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
    open_file(song_widget, fixture_file);
    QVERIFY(!QFile::exists(get_recovery_file_path()));
  };

  void test_recovery_timer_debounce() {
    auto &song_widget = song_editor.song_widget;
    auto &gain_editor = song_widget.controls_column.spin_boxes.gain_editor;
    auto &recovery_timer = song_widget.recovery_timer;

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
  };

  void test_recovery_restore_accepted() {
    auto &song_widget = song_editor.song_widget;
    auto fixture_file = test_dir.filePath("test_song.xml");

    open_file(song_widget, fixture_file);
    const auto old_gain = get_gain(song_widget);
    QCOMPARE_NE(old_gain, NEW_GAIN_1);

    song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
    write_recovery_file(song_widget);
    // undoing approximates relaunching without the unsaved edit -- the
    // recovery file itself is untouched, since only save/open/import/close
    // clear it, not undo
    song_widget.undo_stack.undo();
    QCOMPARE(get_gain(song_widget), old_gain);

    answer_question_later(song_editor, waiting_for_message,
                          RECOVERY_PROMPT_TEXT, QMessageBox::Yes);
    maybe_restore_recovery(song_widget);

    QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
    QCOMPARE(song_widget.current_file, fixture_file);
    QVERIFY(!song_widget.undo_stack.isClean());
    QVERIFY(!QFile::exists(get_recovery_file_path()));
    QVERIFY(!QSettings().contains("recovery/original_file"));

    open_file(song_widget, fixture_file);
  };

  void test_recovery_restore_declined() {
    auto &song_widget = song_editor.song_widget;
    auto fixture_file = test_dir.filePath("test_song.xml");

    open_file(song_widget, fixture_file);
    const auto old_gain = get_gain(song_widget);
    QCOMPARE_NE(old_gain, NEW_GAIN_1);

    song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
    write_recovery_file(song_widget);
    song_widget.undo_stack.undo();

    answer_question_later(song_editor, waiting_for_message,
                          RECOVERY_PROMPT_TEXT, QMessageBox::No);
    maybe_restore_recovery(song_widget);

    QCOMPARE(get_gain(song_widget), old_gain);
    QCOMPARE(song_widget.current_file, fixture_file);
    QVERIFY(song_widget.undo_stack.isClean());
    QVERIFY(!QFile::exists(get_recovery_file_path()));
    QVERIFY(!QSettings().contains("recovery/original_file"));
  };

  void test_recovery_no_prompt_when_missing() {
    remove_recovery_file();
    QVERIFY(!QFile::exists(get_recovery_file_path()));
    // the class-wide unexpected_message_timer watchdog fails the test if a
    // dialog appears here, so no explicit assertion is needed
    maybe_restore_recovery(song_editor.song_widget);
  };

  void test_starting_control_data() {
    QTest::addColumn<QDoubleSpinBox *>("spin_box_pointer");
    QTest::addColumn<double *>("value_pointer");
    QTest::addColumn<double>("first_value");
    QTest::addColumn<double>("second_value");

    auto &song_widget = song_editor.song_widget;
    auto &song = song_widget.song;
    auto &spin_boxes = song_widget.controls_column.spin_boxes;

    QTest::newRow("key") << &spin_boxes.starting_key_editor
                         << &song.starting_key << STARTING_KEY_1
                         << STARTING_KEY_2;
    QTest::newRow("velocity")
        << &spin_boxes.starting_velocity_editor << &song.starting_velocity
        << STARTING_VELOCITY_1 << STARTING_VELOCITY_2;
    QTest::newRow("tempo") << &spin_boxes.starting_tempo_editor
                           << &song.starting_tempo << STARTING_TEMPO_1
                           << STARTING_TEMPO_2;
  }

  void test_starting_control() {
    QFETCH(QDoubleSpinBox *, spin_box_pointer);
    QFETCH(double *, value_pointer);
    QFETCH(const double, first_value);
    QFETCH(const double, second_value);

    auto &spin_box = get_reference(spin_box_pointer);
    auto &value = get_reference(value_pointer);

    const auto old_value = value;
    QCOMPARE_NE(old_value, first_value);
    QCOMPARE_NE(old_value, second_value);

    // test combining
    spin_box.setValue(first_value);
    QCOMPARE(value, first_value);
    spin_box.setValue(second_value);
    QCOMPARE(value, second_value);
    song_editor.song_widget.undo_stack.undo();
    QCOMPARE(value, old_value);
  };

  static void test_status_data() {
    add_table_columns();
    QTest::addColumn<QString>("status");

    QTest::newRow("pitched note") << RowType::pitched_note_type << 1
                                  << "660 Hz ≈ E5 + 2 cents; Velocity 30; 300 "
                                     "bpm; Start at 600 ms; Duration 200 ms";
    QTest::newRow("unpitched note")
        << RowType::unpitched_note_type << 1
        << "Velocity 30; 300 bpm; Start at 600 ms; Duration 200 ms";
  }

  void test_status() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const QString, status);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole),
             status);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_voice_velocity_ratio_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<RowType>("row_type");
    QTest::addColumn<QString>("status");

    QTest::newRow("pitched voice velocity ratio")
        << QString(
               "<song><gain>1</gain><starting_key>220</starting_key>"
               "<starting_tempo>100</starting_tempo><starting_velocity>10</"
               "starting_velocity><pitched_voices><pitched_voice><name>A</"
               "name><instrument>Marimba</instrument><velocity_ratio>"
               "<numerator>2</numerator></velocity_ratio></pitched_voice></"
               "pitched_voices><unpitched_voices><unpitched_voice><name>D</"
               "name><percussion_set_pointer>Room</percussion_set_pointer>"
               "<midi_number>36</midi_number></unpitched_voice></"
               "unpitched_voices><chords><chord><pitched_notes>"
               "<pitched_note><voice_number>0</voice_number></pitched_note>"
               "</pitched_notes></chord></chords></song>")
        << RowType::pitched_note_type
        << "220 Hz ≈ A3; Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("unpitched voice velocity ratio")
        << QString(
               "<song><gain>1</gain><starting_key>220</starting_key>"
               "<starting_tempo>100</starting_tempo><starting_velocity>10</"
               "starting_velocity><pitched_voices><pitched_voice><name>A</"
               "name><instrument>Marimba</instrument></pitched_voice></"
               "pitched_voices><unpitched_voices><unpitched_voice><name>D</"
               "name><percussion_set_pointer>Room</percussion_set_pointer>"
               "<midi_number>36</midi_number><velocity_ratio><numerator>2</"
               "numerator></velocity_ratio></unpitched_voice></"
               "unpitched_voices><chords><chord><unpitched_notes>"
               "<unpitched_note><voice_number>0</voice_number>"
               "</unpitched_note></unpitched_notes></chord></chords></song>")
        << RowType::unpitched_note_type
        << "Velocity 20; 100 bpm; Start at 0 ms; Duration 600 ms";
  };

  void test_voice_velocity_ratio() {
    // a voice's velocity ratio multiplies into the velocity of every note
    // that uses it, on top of the note's own separate velocity ratio
    QFETCH(const QString, text);
    QFETCH(const RowType, row_type);
    QFETCH(const QString, status);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    open_text(song_widget, text);

    switch_to(song_editor, row_type, 0);
    QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole),
             status);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);

    // velocity ratio should also round-trip through save/load like any
    // other ratio column
    QTemporaryFile temp_save_file;
    QVERIFY(temp_save_file.open());
    temp_save_file.close();
    save_as_file(song_widget, temp_save_file.fileName());
    QVERIFY(get_file_text(temp_save_file.fileName())
                .contains("<velocity_ratio><numerator>2</numerator>"
                          "</velocity_ratio>"));
    QFile(temp_save_file.fileName()).remove();

    // restore the shared fixture
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_set_value_data() {
    add_editable_cell_pairs();
    add_voice_column_pairs();
  };

  void test_set_value() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);
    QFETCH(const int, first_row_number);
    QFETCH(const int, second_row_number);
    QFETCH(const int, column_number);

    switch_to(song_editor, row_type, chord_number);

    auto &model = get_model(switch_table);
    const auto second_index = model.index(second_row_number, column_number);

    // use EditRole rather than the default DisplayRole: for the voice
    // columns, DisplayRole shows the voice's name while EditRole (what the
    // cell editor actually reads/writes) is the underlying voice number, so
    // comparing/round-tripping DisplayRole values would bypass the editor
    const auto first_value =
        model.index(first_row_number, column_number).data(Qt::EditRole);
    const auto second_value = second_index.data(Qt::EditRole);
    QCOMPARE_NE(first_value, second_value);

    auto &delegate = get_reference(switch_table.itemDelegate());
    auto &cell_editor = get_reference(
        delegate.createEditor(&get_reference(switch_table.viewport()),
                              QStyleOptionViewItem(), second_index));
    delegate.setEditorData(&cell_editor, second_index);
    cell_editor.setProperty(
        get_reference(cell_editor.metaObject()).userProperty().name(),
        first_value);
    delegate.setModelData(&cell_editor, &model, second_index);

    QCOMPARE(second_index.data(Qt::EditRole), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(Qt::EditRole), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_set_voice_name_data() {
    QTest::addColumn<RowType>("row_type");
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QString>("new_name");

    QTest::newRow("pitched voice")
        << RowType::pitched_voice_type
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_name_column)
        << "New Pitched Voice Name";
    QTest::newRow("unpitched voice")
        << RowType::unpitched_voice_type
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_name_column)
        << "New Unpitched Voice Name";
  };

  void test_set_voice_name() {
    // unlike other voice columns, names must stay unique (see
    // check_voice_name in Voice.hpp), so this can't share test_set_value's
    // swap-two-existing-values pattern: setting a second row's name to a
    // first row's name would collide and warn
    QFETCH(const RowType, row_type);
    QFETCH(const int, column_number);
    QFETCH(const QString, new_name);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(song_editor, row_type, -1);

    auto &model = get_model(switch_table);
    const auto index = model.index(0, column_number);
    const auto old_value = index.data();
    QCOMPARE_NE(old_value.toString(), new_name);

    auto &delegate = get_reference(switch_table.itemDelegate());
    auto &cell_editor = get_reference(
        delegate.createEditor(&get_reference(switch_table.viewport()),
                              QStyleOptionViewItem(), index));
    delegate.setEditorData(&cell_editor, index);
    cell_editor.setProperty(
        get_reference(cell_editor.metaObject()).userProperty().name(),
        QVariant(new_name));
    delegate.setModelData(&cell_editor, &model, index);

    QCOMPARE(index.data().toString(), new_name);
    undo_stack.undo();
    QCOMPARE(index.data(), old_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_voice_paste_insert_disabled_data() {
    QTest::addColumn<RowType>("row_type");
    QTest::addColumn<int>("column_number");

    QTest::newRow("pitched voice instrument")
        << RowType::pitched_voice_type
        << static_cast<int>(PitchedVoiceColumn::pitched_voice_instrument_column);
    QTest::newRow("unpitched voice percussion set")
        << RowType::unpitched_voice_type
        << static_cast<int>(UnpitchedVoiceColumn::unpitched_voice_percussion_set_column);
  };

  void test_voice_paste_insert_disabled() {
    // pasting after/into always inserts a brand new row built only from the
    // pasted column(s), which for voices would create one with an empty
    // (invalid) name -- see ReplaceTable.hpp
    QFETCH(const RowType, row_type);
    QFETCH(const int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &paste_menu = song_editor.song_menu_bar.edit_menu.paste_menu;

    switch_to(song_editor, row_type, -1);
    select_cell(switch_table, 0, column_number);

    QVERIFY(!paste_menu.paste_after_action.isEnabled());
    QVERIFY(!paste_menu.paste_into_start_action.isEnabled());

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_to_string_data() {
    QTest::addColumn<int>("row_number");
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QString>("text");

    QTest::newRow("chord 0 interval")
        << 0 << static_cast<int>(ChordColumn::chord_interval_column) << "";
    QTest::newRow("chord 1 interval")
        << 1 << static_cast<int>(ChordColumn::chord_interval_column) << "3";
    QTest::newRow("chord 2 interval")
        << 2 << static_cast<int>(ChordColumn::chord_interval_column) << "/5";
    QTest::newRow("chord 3 interval")
        << 3 << static_cast<int>(ChordColumn::chord_interval_column) << "3/5";
    QTest::newRow("chord 4 interval")
        << 4 << static_cast<int>(ChordColumn::chord_interval_column) << "o1";
    QTest::newRow("chord 5 interval")
        << FIVE << static_cast<int>(ChordColumn::chord_interval_column) << "3o1";
    QTest::newRow("chord 6 interval")
        << SIX << static_cast<int>(ChordColumn::chord_interval_column) << "/5o1";
    QTest::newRow("chord 7 interval")
        << SEVEN << static_cast<int>(ChordColumn::chord_interval_column) << "3/5o1";
    QTest::newRow("chord 0 beats")
        << 0 << static_cast<int>(ChordColumn::chord_beats_column) << "";
    QTest::newRow("chord 1 beats")
        << 1 << static_cast<int>(ChordColumn::chord_beats_column) << "3";
    QTest::newRow("chord 2 beats")
        << 2 << static_cast<int>(ChordColumn::chord_beats_column) << "/5";
    QTest::newRow("chord 3 beats")
        << 3 << static_cast<int>(ChordColumn::chord_beats_column) << "3/5";
  };

  void test_to_string() {
    QFETCH(const int, row_number);
    QFETCH(const int, column_number);
    QFETCH(const QString, text);

    QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
                 .index(row_number, column_number)
                 .data()
                 .toString(),
             text);
  };

  static void test_unused_role_data() { add_tables(); };

  void test_unused_role() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(song_editor, row_type, chord_number);
    auto &model = get_model(switch_table);
    const auto test_index = model.index(0, 0);
    QCOMPARE(test_index.data(Qt::DecorationRole), QVariant());
    QVERIFY(!(model.setData(test_index, QVariant(), Qt::DecorationRole)));
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_piano_roll_events_data() {
    QTest::addColumn<int>("note_number");
    QTest::addColumn<double>("frequency");
    QTest::addColumn<double>("duration_ms");
    QTest::addColumn<double>("velocity");

    QTest::newRow("pitched note 0") << 0 << 660.0 << 200.0 << 30.0;
    QTest::newRow("pitched note 1") << 1 << 1980.0 << 600.0 << 90.0;
  };

  void test_piano_roll_events() const {
    QFETCH(const int, note_number);
    QFETCH(const double, frequency);
    QFETCH(const double, duration_ms);
    QFETCH(const double, velocity);

    const auto events = get_piano_roll_events(song_editor.song_widget.song);
    const auto matching_event = std::ranges::find_if(
        events, [note_number](const PianoRollNoteEvent &event) -> auto {
          return event.chord_number == 1 && event.note_number == note_number &&
                 event.is_pitched;
        });
    QVERIFY(matching_event != events.cend());
    QCOMPARE(matching_event->start_time_ms, 600.0);
    QCOMPARE(matching_event->duration_ms, duration_ms);
    QCOMPARE(matching_event->frequency, frequency);
    QCOMPARE(matching_event->velocity, velocity);
  };

  void test_piano_roll_events_total_count() const {
    QCOMPARE(get_piano_roll_events(song_editor.song_widget.song).size(), 12);
  };

  void test_piano_roll_time_bounds() const {
    const auto [baseline_ms, end_ms] =
        get_piano_roll_time_bounds(song_editor.song_widget.song, 1, 1);
    QCOMPARE(baseline_ms, 600.0);
    QCOMPARE(end_ms, 1200.0);
  };

  void test_piano_roll_dock_toggle() {
    auto &piano_roll_dock = song_editor.piano_roll_dock;
    auto &show_piano_roll_action =
        song_editor.song_menu_bar.view_menu.show_piano_roll_action;

    // song_editor is never shown() in these headless tests, so isVisible()
    // would always be false regardless of the dock's own shown/hidden state
    // (it also depends on the whole ancestor chain being on-screen);
    // isHidden() reflects the dock's own explicit show/hide state instead.
    QVERIFY(piano_roll_dock.isHidden());
    show_piano_roll_action.trigger();
    QVERIFY(!piano_roll_dock.isHidden());
    show_piano_roll_action.trigger();
    QVERIFY(piano_roll_dock.isHidden());
  };

  void test_piano_roll_rebuilds_on_edit() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &scene = song_editor.piano_roll_widget.piano_roll_view.scene;

    switch_to(song_editor, RowType::pitched_note_type, 1);
    const auto old_item_count = scene.items().size();

    select_cell(switch_table, 0, 0);
    song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action
        .trigger();
    QCOMPARE(scene.items().size(), old_item_count + 1);

    undo_stack.undo(); // undo insert
    QCOMPARE(scene.items().size(), old_item_count);

    maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
  };

  static void test_piano_roll_double_click_selects_note_data() {
    QTest::addColumn<bool>("is_pitched");
    QTest::addColumn<int>("note_number");
    QTest::addColumn<RowType>("expected_row_type");

    QTest::newRow("pitched") << true << 2 << RowType::pitched_note_type;
    QTest::newRow("unpitched") << false << 1 << RowType::unpitched_note_type;
  };

  void test_piano_roll_double_click_selects_note() {
    QFETCH(const bool, is_pitched);
    QFETCH(const int, note_number);
    QFETCH(const RowType, expected_row_type);

    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &undo_stack = song_editor.song_widget.undo_stack;

    // chord number 1 (from test_song.xml) has both pitched and unpitched
    // notes, matching the fixture used by the other piano-roll tests above
    const auto &events = piano_roll_widget.piano_roll_view.events;
    const auto event_iterator = std::ranges::find_if(
        events, [is_pitched, note_number](const PianoRollNoteEvent &event) -> auto {
          return event.chord_number == 1 && event.note_number == note_number &&
                 event.is_pitched == is_pitched;
        });
    QVERIFY(event_iterator != events.cend());
    const auto event_index =
        static_cast<int>(event_iterator - events.cbegin());

    const QGraphicsItem *note_item_pointer = nullptr;
    for (auto *const item_pointer : piano_roll_widget.piano_roll_view.scene.items()) {
      const auto item_data = get_reference(item_pointer).data(0);
      if (item_data.isValid() && item_data.toInt() == event_index) {
        note_item_pointer = item_pointer;
        break;
      }
    }
    QVERIFY(note_item_pointer != nullptr);

    // drives the actual production event filter with a real QMouseEvent,
    // rather than calling add_replace_table directly, so this exercises the
    // full click-to-scene-item-to-callback path
    const auto view_pos = piano_roll_widget.piano_roll_view.view.mapFromScene(
        note_item_pointer->sceneBoundingRect().center());
    const auto global_pos =
        get_reference(piano_roll_widget.piano_roll_view.view.viewport())
            .mapToGlobal(view_pos);
    QMouseEvent double_click_event(
        QEvent::MouseButtonDblClick, QPointF(view_pos), QPointF(global_pos),
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(piano_roll_widget.piano_roll_view.view.viewport(),
                                  &double_click_event);

    QCOMPARE(switch_table.delegate.current_row_type, expected_row_type);
    QCOMPARE(get_parent_chord_number(switch_table), 1);
    QCOMPARE(get_only_range(switch_table).top(), note_number);

    undo_stack.undo();
  };

  static void test_piano_roll_click_selects_note_data() {
    QTest::addColumn<RowType>("row_type");
    QTest::addColumn<bool>("is_pitched");
    QTest::addColumn<int>("note_number");

    QTest::newRow("pitched") << RowType::pitched_note_type << true << 2;
    QTest::newRow("unpitched") << RowType::unpitched_note_type << false << 1;
  };

  void test_piano_roll_click_selects_note() {
    QFETCH(const RowType, row_type);
    QFETCH(const bool, is_pitched);
    QFETCH(const int, note_number);

    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &undo_stack = song_editor.song_widget.undo_stack;

    // enters note mode for chord 1, matching the fixture used by
    // test_piano_roll_double_click_selects_note above, then starts on
    // a different row so the click below has to actually move the
    // selection rather than leave an already-correct one alone
    switch_to(song_editor, row_type, 1);
    select_cell(switch_table, 0, 0);

    const auto &events = piano_roll_widget.piano_roll_view.events;
    const auto event_iterator = std::ranges::find_if(
        events, [is_pitched, note_number](const PianoRollNoteEvent &event) -> auto {
          return event.chord_number == 1 && event.note_number == note_number &&
                 event.is_pitched == is_pitched;
        });
    QVERIFY(event_iterator != events.cend());
    const auto event_index =
        static_cast<int>(event_iterator - events.cbegin());

    const QGraphicsItem *note_item_pointer = nullptr;
    for (auto *const item_pointer : piano_roll_widget.piano_roll_view.scene.items()) {
      const auto item_data = get_reference(item_pointer).data(0);
      if (item_data.isValid() && item_data.toInt() == event_index) {
        note_item_pointer = item_pointer;
        break;
      }
    }
    QVERIFY(note_item_pointer != nullptr);

    // drives the actual production event filter with a real QMouseEvent,
    // the same way test_piano_roll_drag_selects_chord exercises a chord-
    // mode click, so this covers the full click-to-scene-item-to-
    // table-selection path rather than calling select_note_at_bar directly
    const auto view_pos = piano_roll_widget.piano_roll_view.view.mapFromScene(
        note_item_pointer->sceneBoundingRect().center());
    const auto global_pos =
        get_reference(piano_roll_widget.piano_roll_view.view.viewport())
            .mapToGlobal(view_pos);
    QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(view_pos),
                            QPointF(global_pos), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(piano_roll_widget.piano_roll_view.view.viewport(),
                                  &press_event);

    QCOMPARE(switch_table.delegate.current_row_type, row_type);
    QCOMPARE(get_parent_chord_number(switch_table), 1);
    QCOMPARE(get_only_range(switch_table).top(), note_number);

    QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(view_pos),
                              QPointF(global_pos), Qt::NoButton, Qt::NoButton,
                              Qt::NoModifier);
    piano_roll_widget.eventFilter(piano_roll_widget.piano_roll_view.view.viewport(),
                                  &release_event);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_piano_roll_notes_mode_shows_only_chord_notes() {
    auto &song_widget = song_editor.song_widget;
    auto &piano_roll_widget = song_editor.piano_roll_widget;

    // two chords, each with a single note of its own, so entering notes
    // mode for one chord can be checked to hide the other chord's note
    // rather than keep showing every chord's notes on the timeline
    static const QString text =
        "<song><gain>1</gain><starting_key>220</starting_key>"
        "<starting_tempo>100</starting_tempo><starting_velocity>10</"
        "starting_velocity><pitched_voices><pitched_voice><name>A</name>"
        "<instrument>Marimba</instrument></pitched_voice></pitched_voices>"
        "<unpitched_voices><unpitched_voice><name>D</name>"
        "<percussion_set_pointer>Room</percussion_set_pointer><midi_number>36</"
        "midi_number></unpitched_voice></unpitched_voices><chords>"
        "<chord><pitched_notes><pitched_note><voice_number>0</voice_number>"
        "</pitched_note></pitched_notes></chord>"
        "<chord><pitched_notes><pitched_note><voice_number>0</voice_number>"
        "</pitched_note></pitched_notes></chord></chords></song>";
    open_text(song_widget, text);

    QCOMPARE(get_piano_roll_events(song_widget.song).size(), 2);

    switch_to(song_editor, RowType::pitched_note_type, 0);
    QCOMPARE(piano_roll_widget.piano_roll_view.events.size(), 1);
    QCOMPARE(piano_roll_widget.piano_roll_view.events.at(0).chord_number, 0);
    maybe_switch_back_to_chords(song_widget.undo_stack,
                               RowType::pitched_note_type);

    switch_to(song_editor, RowType::pitched_note_type, 1);
    QCOMPARE(piano_roll_widget.piano_roll_view.events.size(), 1);
    QCOMPARE(piano_roll_widget.piano_roll_view.events.at(0).chord_number, 1);
    maybe_switch_back_to_chords(song_widget.undo_stack,
                               RowType::pitched_note_type);

    // back in chord mode, both chords' notes are shown again
    QCOMPARE(piano_roll_widget.piano_roll_view.events.size(), 2);

    // restore the fixture used by the other tests
    open_file(song_widget, test_dir.filePath("test_song.xml"));
  };

  void test_piano_roll_notes_mode_axis_starts_at_chord_start() {
    auto &song_widget = song_editor.song_widget;
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &piano_roll_view = piano_roll_widget.piano_roll_view;
    auto &undo_stack = song_widget.undo_stack;

    // outside notes mode the axis spans the whole song, starting at time 0
    QCOMPARE(piano_roll_view.time_axis_baseline_ms, 0.0);

    // chord 1 starts at 600ms and its notes run through 1200ms (see
    // test_piano_roll_time_bounds() above) -- in notes mode the axis should
    // be rebased to that chord's own start, so it only spans the 600ms
    // during which chord 1's notes actually play rather than dragging along
    // the silent 600ms before them
    switch_to(song_editor, RowType::pitched_note_type, 1);
    QCOMPARE(piano_roll_view.time_axis_baseline_ms, 600.0);
    QCOMPARE(piano_roll_view.time_axis_max_time_ms, 600.0);

    const auto &events = piano_roll_view.events;
    const auto &note_items = piano_roll_view.note_items;
    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      QCOMPARE(get_reference(note_items.at(event_index)).rect().x(),
               (event.start_time_ms - 600.0) * PIANO_ROLL_PIXELS_PER_MS);
    }

    maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
    QCOMPARE(piano_roll_view.time_axis_baseline_ms, 0.0);
  };

  // checks that exactly the events matching the given criteria (mirroring
  // get_selected_piano_roll_event_indices) are drawn with a highlight pen,
  // and every other event is drawn plain
  static void check_piano_roll_highlight(PianoRollWidget &piano_roll_widget,
                                         const RowType selection_row_type,
                                         const int selection_chord_number,
                                         const int selection_note_number) {
    const auto &events = piano_roll_widget.piano_roll_view.events;
    const auto &note_items = piano_roll_widget.piano_roll_view.note_items;
    for (auto event_index = 0; event_index < events.size();
        event_index = event_index + 1) {
      const auto &event = events.at(event_index);
      const auto is_highlighted =
          selection_row_type == RowType::chord_type
              ? event.chord_number == selection_chord_number
              : event.chord_number == selection_chord_number &&
                    event.note_number == selection_note_number &&
                    event.is_pitched ==
                        (selection_row_type == RowType::pitched_note_type);
      QCOMPARE(get_reference(note_items.at(event_index)).pen().style() !=
                   Qt::NoPen,
               is_highlighted);
    }
  }

  void test_piano_roll_selection_highlights_chord() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;

    // chord number 1 (from test_song.xml) has both pitched and unpitched
    // notes, matching the fixture used by the other piano-roll tests above
    select_cell(switch_table, 1, 0);

    check_piano_roll_highlight(piano_roll_widget, RowType::chord_type, 1, -1);

    QVERIFY(piano_roll_widget.piano_roll_view.playhead_item.isVisible());
    QCOMPARE(piano_roll_widget.piano_roll_view.playhead_item.line().x1(),
             600.0 * PIANO_ROLL_PIXELS_PER_MS);
  };

  static void test_piano_roll_selection_highlights_note_data() {
    QTest::addColumn<RowType>("row_type");
    QTest::addColumn<int>("note_number");

    QTest::newRow("pitched") << RowType::pitched_note_type << 2;
    QTest::newRow("unpitched") << RowType::unpitched_note_type << 1;
  };

  void test_piano_roll_selection_highlights_note() {
    QFETCH(const RowType, row_type);
    QFETCH(const int, note_number);

    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &undo_stack = song_editor.song_widget.undo_stack;

    switch_to(song_editor, row_type, 1);
    select_cell(switch_table, note_number, 0);

    check_piano_roll_highlight(piano_roll_widget, row_type, 1, note_number);

    QVERIFY(piano_roll_widget.piano_roll_view.playhead_item.isVisible());
    // both chord 1's pitched and unpitched notes start where the chord
    // itself starts -- see test_piano_roll_time_bounds() above -- but in
    // notes mode the axis is rebased to that same start time (see
    // test_piano_roll_notes_mode_axis_starts_at_chord_start() below), so
    // the playhead sits at 0 rather than at chord 1's absolute start time
    QCOMPARE(piano_roll_widget.piano_roll_view.playhead_item.line().x1(), 0.0);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_piano_roll_selection_ignores_voice_table() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &undo_stack = song_editor.song_widget.undo_stack;

    // put a highlight/cursor up first, so switching to a voice table (which
    // has no timeline position) has to actually clear it rather than just
    // never having set it
    select_cell(switch_table, 1, 0);
    QVERIFY(piano_roll_widget.piano_roll_view.playhead_item.isVisible());

    switch_to(song_editor, RowType::pitched_voice_type, -1);
    select_cell(switch_table, 0, 0);

    QVERIFY(!piano_roll_widget.piano_roll_view.playhead_item.isVisible());
    for (auto *const note_item_pointer : piano_roll_widget.piano_roll_view.note_items) {
      QCOMPARE(get_reference(note_item_pointer).pen().style(), Qt::NoPen);
    }

    maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);
  };

  void test_piano_roll_selection_preserves_multi_row_range() {
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;

    // selecting a range of chords (e.g. for "Play selection") must not get
    // collapsed down to a single row by the piano roll's cursor-follows-
    // selection sync -- regression test for a bug where every table
    // selection change (not just the cursor actually moving) fed back
    // through select_chord_at_playhead() and forced a single-row reselect
    auto &model = get_model(switch_table);
    get_selection_model(switch_table)
        .select(QItemSelection(model.index(1, 0), model.index(3, 0)),
                SELECT_AND_CLEAR);

    const auto &range = get_only_range(switch_table);
    QCOMPARE(range.top(), 1);
    QCOMPARE(range.bottom(), 3);
  };

  void test_piano_roll_drag_selects_chord() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;

    // start on chord 1 (600ms-1200ms, per test_piano_roll_time_bounds()
    // above) so the drag below has to actually move the selection rather
    // than leave an already-correct one alone
    select_cell(switch_table, 1, 0);

    // chord 2 starts where chord 1 ends, at 1200ms
    const auto view_pos = piano_roll_widget.piano_roll_view.view.mapFromScene(
        QPointF(1200.0 * PIANO_ROLL_PIXELS_PER_MS, 0));
    const auto global_pos =
        get_reference(piano_roll_widget.piano_roll_view.view.viewport())
            .mapToGlobal(view_pos);

    QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(view_pos),
                            QPointF(global_pos), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(piano_roll_widget.piano_roll_view.view.viewport(),
                                  &press_event);

    QCOMPARE(switch_table.delegate.current_row_type, RowType::chord_type);
    QCOMPARE(get_only_range(switch_table).top(), 2);

    QMouseEvent release_event(QEvent::MouseButtonRelease, QPointF(view_pos),
                              QPointF(global_pos), Qt::NoButton, Qt::NoButton,
                              Qt::NoModifier);
    piano_roll_widget.eventFilter(piano_roll_widget.piano_roll_view.view.viewport(),
                                  &release_event);
  };

  void test_piano_roll_drag_selects_chord_range() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;
    auto &view = piano_roll_widget.piano_roll_view.view;
    auto &selection_rect_item = piano_roll_widget.piano_roll_view.selection_rect_item;
    const auto &song = song_editor.song_widget.song;

    // start on chord 0 so the drag below has to actually move the
    // selection rather than leave an already-correct one alone
    select_cell(switch_table, 0, 0);

    // the box mirrors whatever's selected, so it's already showing chord
    // 0's own extent before any drag happens
    {
      const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 0, 1);
      QVERIFY(selection_rect_item.isVisible());
      QCOMPARE(selection_rect_item.rect().left(), start_ms * PIANO_ROLL_PIXELS_PER_MS);
      QCOMPARE(selection_rect_item.rect().right(), end_ms * PIANO_ROLL_PIXELS_PER_MS);
    }

    const auto &chord_start_times =
        piano_roll_widget.piano_roll_view.chord_start_times;
    QVERIFY(chord_start_times.size() > 3);

    const auto press_scene_x = chord_start_times.at(1) * PIANO_ROLL_PIXELS_PER_MS;
    const auto press_view_pos =
        view.mapFromScene(QPointF(press_scene_x, 0));
    const auto press_global_pos =
        get_reference(view.viewport()).mapToGlobal(press_view_pos);

    QMouseEvent press_event(QEvent::MouseButtonPress, QPointF(press_view_pos),
                            QPointF(press_global_pos), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(view.viewport(), &press_event);

    QCOMPARE(get_only_range(switch_table).top(), 1);
    QCOMPARE(get_only_range(switch_table).bottom(), 1);

    // the box follows the newly (single-chord) selection
    {
      const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 1);
      QVERIFY(selection_rect_item.isVisible());
      QCOMPARE(selection_rect_item.rect().left(), start_ms * PIANO_ROLL_PIXELS_PER_MS);
      QCOMPARE(selection_rect_item.rect().right(), end_ms * PIANO_ROLL_PIXELS_PER_MS);
    }

    const auto move_scene_x = chord_start_times.at(3) * PIANO_ROLL_PIXELS_PER_MS;
    const auto move_view_pos = view.mapFromScene(QPointF(move_scene_x, 0));
    const auto move_global_pos =
        get_reference(view.viewport()).mapToGlobal(move_view_pos);

    QMouseEvent move_event(QEvent::MouseMove, QPointF(move_view_pos),
                           QPointF(move_global_pos), Qt::NoButton,
                           Qt::LeftButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(view.viewport(), &move_event);

    // dragging from chord 1 to chord 3 should select the whole range in
    // between, not just reassign the selection to chord 3 alone
    QCOMPARE(get_only_range(switch_table).top(), 1);
    QCOMPARE(get_only_range(switch_table).bottom(), 3);

    // the box now spans the whole selected chord range
    {
      const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 3);
      QVERIFY(selection_rect_item.isVisible());
      QCOMPARE(selection_rect_item.rect().left(), start_ms * PIANO_ROLL_PIXELS_PER_MS);
      QCOMPARE(selection_rect_item.rect().right(), end_ms * PIANO_ROLL_PIXELS_PER_MS);
    }

    QMouseEvent release_event(QEvent::MouseButtonRelease,
                              QPointF(move_view_pos), QPointF(move_global_pos),
                              Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    piano_roll_widget.eventFilter(view.viewport(), &release_event);

    // the box mirrors the committed table selection rather than being a
    // purely in-drag affordance, so it must still be showing the same
    // range after the mouse is released
    {
      const auto [start_ms, end_ms] = get_piano_roll_time_bounds(song, 1, 3);
      QVERIFY(selection_rect_item.isVisible());
      QCOMPARE(selection_rect_item.rect().left(), start_ms * PIANO_ROLL_PIXELS_PER_MS);
      QCOMPARE(selection_rect_item.rect().right(), end_ms * PIANO_ROLL_PIXELS_PER_MS);
    }
  };

  void test_piano_roll_playback_selects_chord() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;
    auto &switch_table = song_editor.song_widget.switch_column.switch_table;

    // start on chord 0; starting playback at chord 2's baseline (1200ms,
    // per test_piano_roll_time_bounds() above) shouldn't itself move the
    // selection -- only a timer tick should, so a multi-chord "Play
    // selection" isn't collapsed to a single row the instant Play is
    // pressed
    select_cell(switch_table, 0, 0);

    start_piano_roll_playhead(piano_roll_widget, 1200.0, 1800.0);
    QCOMPARE(get_only_range(switch_table).top(), 0);

    // simulates one playback timer tick without waiting on the real
    // QElapsedTimer -- elapsed() is at least 0, so current_ms is already
    // >= the 1200ms baseline, landing on chord 2
    update_playhead_position(piano_roll_widget.piano_roll_view,
                             piano_roll_widget.axis_view, switch_table,
                             piano_roll_widget.selecting_chord_from_playhead);
    QCOMPARE(get_only_range(switch_table).top(), 2);

    stop_piano_roll_playhead(piano_roll_widget);
  };

  void test_piano_roll_zoom() {
    auto &piano_roll_widget = song_editor.piano_roll_widget;

    QCOMPARE(piano_roll_widget.piano_roll_view.view.transform().m11(), 1.0);
    QCOMPARE(piano_roll_widget.piano_roll_view.view.transform().m22(), 1.0);

    zoom_in_piano_roll(piano_roll_widget);
    // only the time (x) axis scales -- the pitch (y) axis has to stay fixed
    // so it stays aligned with axis_view, which is never zoomed
    QCOMPARE(piano_roll_widget.piano_roll_view.view.transform().m11(),
            PIANO_ROLL_TIME_ZOOM_STEP);
    QCOMPARE(piano_roll_widget.piano_roll_view.view.transform().m22(), 1.0);

    zoom_out_piano_roll(piano_roll_widget);
    QCOMPARE(piano_roll_widget.piano_roll_view.view.transform().m11(), 1.0);

    // clamped rather than unbounded, so repeated zooming can't shrink/grow
    // the time axis into something unusable
    for (auto zoom_count = 0; zoom_count < 20; zoom_count = zoom_count + 1) {
      zoom_out_piano_roll(piano_roll_widget);
    }
    QCOMPARE(piano_roll_widget.piano_roll_view.time_zoom_factor, PIANO_ROLL_MIN_TIME_ZOOM);

    for (auto zoom_count = 0; zoom_count < 40; zoom_count = zoom_count + 1) {
      zoom_in_piano_roll(piano_roll_widget);
    }
    QCOMPARE(piano_roll_widget.piano_roll_view.time_zoom_factor, PIANO_ROLL_MAX_TIME_ZOOM);

    // restore, so later tests see the default 1x zoom
    set_notes_view_time_zoom(piano_roll_widget.piano_roll_view, 1.0);
  };
};