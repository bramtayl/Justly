#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTimer>
#include <QtCore/QTypeInfo>
#include <QtCore/Qt>
#include <QtCore/QtAssert>
#include <QtCore/qobjectdefs.h>
#include <QtGui/QAction>
#include <QtGui/QUndoStack>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>
#include <QtTest/qtestkeyboard.h>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <memory>
#include <string>
#include <utility>

#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "other/helpers.hpp"
#include "rows/RowType.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

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

inline auto get_model(QAbstractItemView &table) -> auto & {
  return get_reference(table.model());
}

inline void double_click_column(QAbstractItemView &table, const int row_number,
                                const int column_number) {
  table.doubleClicked(get_model(table).index(row_number, column_number));
}

inline void select_cell(QAbstractItemView &table, const int row,
                        const int column) {
  get_selection_model(table).select(get_model(table).index(row, column),
                                    SELECT_AND_CLEAR);
}

inline void switch_to(SongEditor &song_editor, const RowType row_type,
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

inline void maybe_switch_back_to_chords(QUndoStack &undo_stack,
                                        const RowType row_type) {
  if (row_type != RowType::chord_type) {
    undo_stack.undo();
  }
}

inline void open_text(SongEditor &song_editor, const QString &song_text) {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(song_text.toStdString().c_str());
  temp_file.close();
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, temp_file.fileName());
}

// builds a minimal <song> fixture with one pitched/unpitched voice per given
// name and, optionally, one <chord> per entry in chord_voice_numbers whose
// pitched/unpitched notes reference voices by number (first/second of the
// pair respectively; an empty list omits that note type from the chord).
// Instrument/percussion_set/midi_number are arbitrary, since no voice test
// asserts on them -- only on voice names, counts, and voice_numbers.
inline auto make_voice_song_xml(
    const QList<QString> &pitched_voice_names,
    const QList<QString> &unpitched_voice_names,
    const QList<std::pair<QList<int>, QList<int>>> &chord_voice_numbers = {})
    -> QString {
  QString xml = "<song><gain>1</gain><starting_key>220</starting_key>"
               "<starting_tempo>100</starting_tempo><starting_velocity>10</"
               "starting_velocity><pitched_voices>";
  for (const auto &name : pitched_voice_names) {
    xml += "<pitched_voice><name>" + name +
          "</name><instrument>Marimba</instrument></pitched_voice>";
  }
  xml += "</pitched_voices><unpitched_voices>";
  auto midi_number = 36;
  for (const auto &name : unpitched_voice_names) {
    xml += "<unpitched_voice><name>" + name +
          "</name><percussion_set_pointer>Room</percussion_set_pointer>"
          "<midi_number>" +
          QString::number(midi_number) +
          "</midi_number></unpitched_voice>";
    midi_number += 1;
  }
  xml += "</unpitched_voices>";
  if (!chord_voice_numbers.isEmpty()) {
    xml += "<chords>";
    for (const auto &[pitched_numbers, unpitched_numbers] :
        chord_voice_numbers) {
      xml += "<chord>";
      if (!pitched_numbers.isEmpty()) {
        xml += "<pitched_notes>";
        for (const auto voice_number : pitched_numbers) {
          xml += "<pitched_note><voice_number>" +
                QString::number(voice_number) +
                "</voice_number></pitched_note>";
        }
        xml += "</pitched_notes>";
      }
      if (!unpitched_numbers.isEmpty()) {
        xml += "<unpitched_notes>";
        for (const auto voice_number : unpitched_numbers) {
          xml += "<unpitched_note><voice_number>" +
                QString::number(voice_number) +
                "</voice_number></unpitched_note>";
        }
        xml += "</unpitched_notes>";
      }
      xml += "</chord>";
    }
    xml += "</chords>";
  }
  xml += "</song>";
  return xml;
}

[[nodiscard]] inline auto find_top_level_message_box() -> QMessageBox * {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *const box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
    if (box_pointer != nullptr && box_pointer->isVisible()) {
      return box_pointer;
    }
  }
  return nullptr;
}

[[nodiscard]] inline auto find_top_level_file_dialog() -> QFileDialog * {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *const dialog_pointer = dynamic_cast<QFileDialog *>(widget_pointer);
    if (dialog_pointer != nullptr && dialog_pointer->isVisible()) {
      return dialog_pointer;
    }
  }
  return nullptr;
}

// simulates a user picking a file and accepting a Save/Export dialog,
// exercising FileMenu's dialog-accept lambdas (get_selected_file and
// whatever it hands the selected path to) instead of only the reject path
// test_file_dialog_cleanup drives
inline void accept_file_dialog_later(QWidget &parent,
                                     const QString &file_path) {
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&parent));
  timer.setSingleShot(true);
  QObject::connect(&timer, &QTimer::timeout, &parent,
                   [file_path]() -> auto {
                     auto *const found_dialog = find_top_level_file_dialog();
                     QVERIFY(found_dialog != nullptr);
                     // selectFile() only sets the directory when the target
                     // doesn't exist yet (the async QFileSystemModel has
                     // nothing to match it against) -- type into the
                     // filename line edit directly instead, exactly like a
                     // user picking a new Save/Export filename would, then
                     // press Enter in that field (accept() itself is a
                     // protected override, not directly callable)
                     auto *const line_edit = found_dialog->findChild<QLineEdit *>();
                     QVERIFY(line_edit != nullptr);
                     line_edit->setText(file_path);
                     QTest::keyEvent(QTest::Press, line_edit, Qt::Key_Enter);
                   });
  timer.start(WAIT_TIME);
}

inline void close_message_later(QWidget &parent, bool &waiting_for_message,
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
inline void
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
inline void answer_question_later(QWidget &parent, bool &waiting_for_message,
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

inline void press_times(QPushButton &plus_button, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    plus_button.click();
  }
}

inline void undo_times(QUndoStack &undo_stack, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    undo_stack.undo();
  }
}

inline void add_table_columns() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("chord_number");
}

inline void add_tables() {
  add_table_columns();

  QTest::newRow("chord") << RowType::chord_type << -1;
  QTest::newRow("pitched note") << RowType::pitched_note_type << 1;
  QTest::newRow("unpitched note") << RowType::unpitched_note_type << 1;
  QTest::newRow("pitched voice") << RowType::pitched_voice_type << -1;
  QTest::newRow("unpitched voice") << RowType::unpitched_voice_type << -1;
}

inline void add_cells() {
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

inline void add_editable_cell_pairs() {
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
inline void add_voice_column_pairs() {
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

inline void add_cell_pairs() {
  add_editable_cell_pairs();
  QTest::newRow("chord pitched notes")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_pitched_notes_column);
  QTest::newRow("chord unpitched notes")
      << RowType::chord_type << -1 << 0 << 1
      << static_cast<int>(ChordColumn::chord_unpitched_notes_column);
}

inline auto get_file_text(const QString &filename) {
  QFile file(filename);
  auto opened = file.open(QIODevice::ReadOnly);
  Q_ASSERT(opened);
  QString file_text(file.readAll());
  file.close();
  // normalize line endings
  file_text.replace("\r\n", "\n");
  return file_text;
}
