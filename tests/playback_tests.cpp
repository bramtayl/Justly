#include <QtCore/QAbstractItemModel>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QThread>
#include <QtCore/QTypeInfo>
#include <QtGui/QAction>
#include <QtTest/QTestData>
#include <QtTest/qtestcase.h>

#include "Tester.hpp"
#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "rows/RowType.hpp"
#include "test_helpers.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/piano_roll/PianoRollNotesScene.hpp"
#include "widgets/piano_roll/PianoRollWidget.hpp"

void Tester::test_play_data() {

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
}

void Tester::test_play() {
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
}

void Tester::test_play_to_end_starts_playhead() {
  // regression test: "Play to end" is a separate action from "Play
  // selection" and must independently start the piano roll playhead
  // animation, not just trigger audio playback
  auto &song_widget = song_editor.song_widget;
  auto &piano_roll_widget = song_editor.piano_roll_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &play_menu = song_editor.song_menu_bar.play_menu;

  select_cell(switch_table, 0, 0);

  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_active);
  play_menu.play_to_end_action.trigger();
  QVERIFY(piano_roll_widget.piano_roll_scene.playhead_active);

  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();
  QVERIFY(!piano_roll_widget.piano_roll_scene.playhead_active);
}

// unlike test_play, only exercises chord_type/pitched_note_type/
// unpitched_note_type -- play_to_end_action has no voice_type branch, it
// Q_ASSERTs false for anything else
void Tester::test_play_to_end_data() {
  add_table_columns();
  QTest::addColumn<int>("row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord")
      << RowType::chord_type << -1 << 1
      << static_cast<int>(ChordColumn::chord_interval_column);
  QTest::newRow("pitched note")
      << RowType::pitched_note_type << 1 << 1
      << static_cast<int>(PitchedNoteColumn::pitched_note_voice_number_column);
  QTest::newRow("unpitched note")
      << RowType::unpitched_note_type << 1 << 1
      << static_cast<int>(UnpitchedNoteColumn::unpitched_note_voice_number_column);
}

void Tester::test_play_to_end() {
  QFETCH(const RowType, row_type);
  QFETCH(const int, chord_number);
  QFETCH(const int, row_number);
  QFETCH(const int, column_number);

  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &play_menu = song_editor.song_menu_bar.play_menu;

  switch_to(song_editor, row_type, chord_number);

  select_cell(switch_table, row_number, column_number);
  play_menu.play_to_end_action.trigger();
  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();

  maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
}
