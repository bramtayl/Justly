#include <QtTest/qtestcase.h>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QtMinMax>
#include <QtCore/QtSwap>
#include <QtGui/QAction>
#include <QtTest/QTest>
#include <QtTest/QTestData>
#include <QtWidgets/QLabel>
#include <utility>
#include <iterator>

#include "Tester.hpp"
#include "cell_types/Rational.hpp"
#include "menus/EditMenu.hpp"
#include "menus/FileMenu.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "musicxml/MeasureRepeatInfo.hpp"
#include "other/Song.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/RowType.hpp"
#include "test_helpers.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

void Tester::test_musicxml_data() {
  QTest::addColumn<QString>("file_name");
  QTest::addColumn<int>("number_of_chords");

  QTest::newRow("prelude") << "prelude.musicxml" << MUSIC_XML_ROWS;
  QTest::newRow("compressed prelude") << "prelude.mxl" << MUSIC_XML_ROWS;
  QTest::newRow("percussion") << "percussion.musicxml" << PERCUSSION_ROWS;
  QTest::newRow("transposing instruments")
      << "MozartTrio.musicxml" << MOZART_ROWS;
  QTest::newRow("repeats") << "Saltarello.musicxml" << SALTARELLO_ROWS;
}

void Tester::test_musicxml() {
  QFETCH(const QString, file_name);
  QFETCH(const int, number_of_chords);

  auto &song_widget = song_editor.song_widget;

  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget, test_dir.filePath(file_name));
  QCOMPARE(get_model(song_widget.switch_column.switch_table)
               .rowCount(QModelIndex()),
           number_of_chords);
  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, test_dir.filePath("test_song.xml"));
}

void Tester::test_musicxml_error_data() {
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
  // regression test: divisions is unbounded xs:decimal with no
  // schema-enforced range, so a magnitude that doesn't fit in a 32-bit int
  // must be rejected with a warning instead of overflowing std::stoi
  QTest::newRow("overflowing divisions")
      << "overflowing_divisions.musicxml"
      << "Divisions value is out of range";
}

void Tester::test_musicxml_error() {
  QFETCH(const QString, error_message);
  QFETCH(const QString, file_name);

  close_message_later(song_editor, waiting_for_message, error_message);
  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget, test_dir.filePath(file_name));
}

// regression test: a backward repeat with no forward repeat since the
// last block boundary (e.g. a repeat that implicitly continues right
// after an earlier, already-expanded repeated section) must replay from
// that block boundary, not from measure 0 -- otherwise an earlier,
// already-consumed repeated section gets incorrectly replayed again, and
// the plain measures between the two sections get silently dropped
// instead of flushed
void Tester::test_compute_measure_expansion_lone_backward_repeat() {
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
}

// regression test: the musicxml importer's in-progress-tie lookup used to
// be keyed by pitch alone, shared across every voice (and every part) in
// the whole document. Two simultaneous voices tying the same pitch (here,
// two instruments in one piano part) would clobber each other's still-open
// note: the second voice's tie-start silently overwrote the first voice's
// map entry, so the first voice's tie-stop resolved onto the wrong note
// (wrong words/duration) and the second voice's own tie-stop then found no
// entry at all. Keying the lookup by voice as well as pitch keeps
// overlapping ties on the same pitch independent.
void Tester::test_import_musicxml_ties_do_not_cross_voices() {
  auto &song_widget = song_editor.song_widget;

  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget, test_dir.filePath("tied_voices.musicxml"));

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

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, test_dir.filePath("test_song.xml"));
}

// regression test: a <tie type="stop"/> with no matching earlier
// <tie type="start"/> for the same pitch/voice used to dereference a
// missing map entry, guarded only by a release-mode-noop Q_ASSERT -- a
// malformed or hand-edited musicxml file could trigger undefined behavior.
// It must now import as an ordinary, unstarted note instead.
void Tester::test_import_musicxml_orphan_tie_stop() {
  auto &song_widget = song_editor.song_widget;

  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget, test_dir.filePath("orphan_tie.musicxml"));

  auto &song = song_widget.song;
  QCOMPARE(song.chords.size(), 1);

  const auto &notes = song.chords.at(0).pitched_notes;
  QCOMPARE(notes.size(), 1);
  QCOMPARE(notes.at(0).beats.numerator, 4);
  QCOMPARE(notes.at(0).beats.denominator, 1);

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, test_dir.filePath("test_song.xml"));
}

// regression test: inserting a chord, then drilling into and inserting one
// of its notes, leaves the switch table's notes model pointing directly at
// that Chord's notes QList; import_musicxml/open_file must not crash even
// though they replace song.chords wholesale while that pointer is live
void Tester::test_import_musicxml_after_editing_chord_notes() {
  auto &song_widget = song_editor.song_widget;
  auto &switch_table = song_widget.switch_column.switch_table;
  auto &insert_menu = song_editor.song_menu_bar.edit_menu.insert_menu;

  select_cell(switch_table, 0, 0);
  insert_menu.insert_after_action.trigger();
  switch_to(song_editor, RowType::pitched_note_type, 1);
  insert_menu.insert_into_start_action.trigger();

  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget, test_dir.filePath("prelude.musicxml"));
  QCOMPARE(get_model(switch_table).rowCount(QModelIndex()), MUSIC_XML_ROWS);

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget, test_dir.filePath("test_song.xml"));
}

// regression test: opening/importing a file bypasses the undo stack, so
// replace_table (which normally resets the switch column's label and the
// view menu's actions whenever the table switches to a different row
// type) never runs for them; open_file_and_reload/import_musicxml_and_reload
// must reapply that same reset explicitly (via song_reloaded) instead of
// leaving the label/actions stuck showing whatever was being edited before
void Tester::test_open_after_editing_chord_notes_resets_menu() {
  auto &song_widget = song_editor.song_widget;
  auto &switch_column = song_widget.switch_column;
  auto &view_menu = song_editor.song_menu_bar.view_menu;

  switch_to(song_editor, RowType::pitched_note_type, 0);
  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");
  QVERIFY(view_menu.back_to_chords_action.isEnabled());

  open_file_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                       song_editor.piano_roll_widget,
                       test_dir.filePath("test_song.xml"));

  QCOMPARE(switch_column.editing_text.text(), "Chords");
  QVERIFY(!view_menu.back_to_chords_action.isEnabled());
  QVERIFY(view_menu.edit_pitched_voices_action.isEnabled());
  QVERIFY(view_menu.edit_unpitched_voices_action.isEnabled());
  QVERIFY(!view_menu.previous_chord_action.isEnabled());
  QVERIFY(!view_menu.next_chord_action.isEnabled());
}

// regression test: File > Open used to be disabled while viewing anything
// other than chords (pitched/unpitched notes or voices), even though
// there's no reason opening a different file requires navigating back to
// the chords view first
void Tester::test_open_action_enabled_outside_chords_view() {
  auto &undo_stack = song_editor.song_widget.undo_stack;
  auto &open_action = song_editor.song_menu_bar.file_menu.open_action;

  QVERIFY(open_action.isEnabled());

  switch_to(song_editor, RowType::pitched_voice_type, -1);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_voice_type);

  switch_to(song_editor, RowType::pitched_note_type, 0);
  QVERIFY(open_action.isEnabled());
  maybe_switch_back_to_chords(undo_stack, RowType::pitched_note_type);
}

// regression test: song_reloaded (which resets the switch column's label/
// view menu and rebuilds the piano roll) must only run once open_file/
// import_musicxml actually succeed -- a rejected file must leave whatever
// view the user was on (including mid-note-editing) completely alone
// rather than silently bouncing them back to the chords view
void Tester::test_failed_import_does_not_reset_notes_view() {
  auto &song_widget = song_editor.song_widget;
  auto &switch_column = song_widget.switch_column;

  switch_to(song_editor, RowType::pitched_note_type, 0);
  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");

  close_message_later(song_editor, waiting_for_message,
                      "Invalid musicxml file");
  import_musicxml_and_reload(song_editor.song_menu_bar, song_editor.song_widget,
                             song_editor.piano_roll_widget,
                             test_dir.filePath("not_musicxml.xml"));

  QCOMPARE(switch_column.editing_text.text(), "Pitched notes for chord 1");

  maybe_switch_back_to_chords(song_widget.undo_stack,
                             RowType::pitched_note_type);
}
