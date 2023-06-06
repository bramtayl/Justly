#include "Tester.h"

#include <QtCore/qglobal.h>      // for QFlags
#include <bits/chrono.h>         // for milliseconds
#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qcolor.h>              // for QColor
#include <qnamespace.h>          // for ForegroundRole, operator|, Decoratio...
#include <qtestcase.h>           // for qCompare, QCOMPARE, QVERIFY
#include <qtextstream.h>         // for QTextStream
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <QApplication>
#include <QMessageBox>
#include <QTest>
#include <QTimer>
#include <memory>  // for unique_ptr
#include <thread>  // for sleep_for

#include "Chord.h"      // for CHORD_LEVEL
#include "Note.h"       // for NOTE_LEVEL
#include "NoteChord.h"  // for symbol_column, numerator_column, bea...
#include "Song.h"       // for Song, DEFAULT_DEFAULT_INSTRUMENT
#include "TreeNode.h"   // for TreeNode, ROOT_LEVEL
#include "Utilities.h"  // for cannot_open_error, assert_not_empty

const auto NEW_FREQUENCY = 401;
const auto NEW_TEMPO = 221;
const auto NEW_STARTING_VOLUME_PERCENT = 51;
const auto TWO_DOUBLE = 2.0;

const auto WAIT_TIME = 3000;

const auto LIGHT_GRAY = QColor(Qt::lightGray);

const auto NO_DATA = QVariant();

const auto MESSAGE_BOX_WAIT = 500;

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor.song.headerData(column, Qt::Horizontal, Qt::DisplayRole);
}

auto Tester::get_data(int row, int column, QModelIndex &parent_index,
                      Qt::ItemDataRole role) -> QVariant {
  auto &song = editor.song;
  return song.data(song.index(row, column, parent_index), role);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) -> bool {
  auto &song = editor.song;
  return song.setData(song.index(row, column, parent_index), new_value,
                      Qt::EditRole);
}

void Tester::initTestCase() {
  if (test_file.open()) {
    QTextStream test_io(&test_file);
    test_io << R""""(
{
    "children": [
        {
            "children": [
                {
                },
                {
                    "numerator": 2,
                    "denominator": 2,
                    "octave": 1,
                    "beats": 2,
                    "volume_percent": 2.0,
                    "tempo_percent": 2.0,
                    "words": "hello",
                    "instrument": "Wurley"
                }
            ]
        },
        {
            "numerator": 2,
            "denominator": 2,
            "octave": 1,
            "beats": 2,
            "volume_percent": 2.0,
            "tempo_percent": 2.0,
            "words": "hello",
            "children": []
        }
    ],
    "default_instrument": "Plucked",
    "frequency": 220,
    "orchestra_text": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "tempo": 200,
    "volume_percent": 50
}
    )"""";
    test_file.close();

  } else {
    cannot_open_error(test_file.fileName());
  }
  editor.load_from(test_file.fileName());
}

void Tester::test_column_headers() {
  auto &song = editor.song;
  // no symbol header
  QCOMPARE(get_column_heading(symbol_column), QVariant());
  QCOMPARE(get_column_heading(numerator_column), "Numerator");
  QCOMPARE(get_column_heading(denominator_column), "Denominator");
  QCOMPARE(get_column_heading(octave_column), "Octave");
  QCOMPARE(get_column_heading(beats_column), "Beats");
  QCOMPARE(get_column_heading(volume_percent_column), "Volume Percent");
  QCOMPARE(get_column_heading(tempo_percent_column), "Tempo Percent");
  QCOMPARE(get_column_heading(words_column), "Words");
  QCOMPARE(get_column_heading(instrument_column), "Instrument");
  // error for non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(song.headerData(numerator_column, Qt::Vertical, Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(
      song.headerData(numerator_column, Qt::Horizontal, Qt::DecorationRole),
      QVariant());
}

void Tester::test_save() {
  editor.song.save_to(test_file.fileName());
  QTest::ignoreMessage(QtCriticalMsg, "Cannot open file not_a_file");
  cannot_open_error("not_a_file");
}

void Tester::test_insert_delete() {
  auto root_index = QModelIndex();
  auto &undo_stack = editor.undo_stack;
  auto first_chord_symbol_index = 
      editor.song.index(0, symbol_column, root_index);
  auto first_chord_instrument_index = editor.song.index(0, instrument_column, root_index);
  auto &first_chord_node = editor.song.root.get_child(0);
  auto first_note_symbol_index =
      editor.song.index(0, symbol_column, first_chord_symbol_index);
  auto first_note_instrument_index =
      editor.song.index(0, instrument_column, first_chord_symbol_index);
  auto &first_note_node = first_chord_node.get_child(0);
  auto second_chord_symbol_index =
      editor.song.index(1, symbol_column, root_index);
  auto second_chord_instrument_index =
      editor.song.index(1, instrument_column, root_index);
  auto &second_chord_node = editor.song.root.get_child(1);

  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.copy_selected();
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.copy_selected();
  


  // paste after first chord
  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.paste_before();
  QCOMPARE(editor.song.root.child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.root.child_pointers.size(), 2);
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.paste_before();


  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.paste_after();
  QCOMPARE(editor.song.root.child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.root.child_pointers.size(), 2);
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.paste_after();

  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.insert_before();
  QCOMPARE(editor.song.root.child_pointers.size(), 3);
  undo_stack.undo();
  QCOMPARE(editor.song.root.child_pointers.size(), 2);
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.insert_before();

  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.insert_after();
  QCOMPARE(editor.song.root.child_pointers.size(), 3);
  undo_stack.undo();
  QCOMPARE(editor.song.root.child_pointers.size(), 2);
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QCOMPARE(editor.selector.selectedRows().size(), 0);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.insert_after();

  select_indices(second_chord_symbol_index, second_chord_instrument_index);
  editor.insert_into();
  QCOMPARE(second_chord_node.child_pointers.size(), 1);
  undo_stack.undo();
  QCOMPARE(second_chord_node.child_pointers.size(), 0);
  clear_indices(second_chord_symbol_index, second_chord_instrument_index);
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor.insert_into();

  select_indices(first_chord_symbol_index, first_chord_instrument_index);
  editor.remove_selected();
  QCOMPARE(editor.song.root.child_pointers.size(), 1);
  undo_stack.undo();
  QCOMPARE(editor.song.root.child_pointers.size(), 2);
  clear_indices(first_chord_symbol_index, first_chord_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.remove_selected();

  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.copy_selected();
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.copy_selected();

  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.paste_before();
  QCOMPARE(first_chord_node.child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.paste_before();

  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.paste_after();
  QCOMPARE(first_chord_node.child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.paste_after();

  select_indices(second_chord_symbol_index, second_chord_instrument_index);
  editor.paste_into();
  QCOMPARE(second_chord_node.child_pointers.size(), 1);
  undo_stack.undo();
  QCOMPARE(second_chord_node.child_pointers.size(), 0);
  clear_indices(second_chord_symbol_index, second_chord_instrument_index);
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor.paste_into();

  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.insert_before();
  QCOMPARE(first_chord_node.child_pointers.size(), 3);
  undo_stack.undo();
  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.insert_before();

  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.insert_after();
  QCOMPARE(first_chord_node.child_pointers.size(), 3);
  undo_stack.undo();
  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.insert_after();

  select_indices(first_note_symbol_index, first_note_instrument_index);
  editor.remove_selected();
  QCOMPARE(first_chord_node.child_pointers.size(), 1);
  undo_stack.undo();
  QCOMPARE(first_chord_node.child_pointers.size(), 2);
  clear_indices(first_note_symbol_index, first_note_instrument_index);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  editor.remove_selected();
}

void Tester::test_play() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index =
      editor.song.index(0, symbol_column, root_index);
  auto second_chord_instrument_index =
      editor.song.index(1, instrument_column, root_index);
  auto second_chord_symbol_index = song.index(1, symbol_column, root_index);
  select_indices(first_chord_symbol_index, second_chord_instrument_index);

  // use the second chord to test key changing
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));

  clear_indices(first_chord_symbol_index, second_chord_instrument_index);

  auto second_note_symbol_index =
      song.index(1, symbol_column, first_chord_symbol_index);
  auto second_note_instrument_index =
      song.index(1, symbol_column, first_chord_symbol_index);
  select_indices(second_note_symbol_index, second_note_instrument_index);

  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
  clear_indices(second_note_symbol_index, second_note_instrument_index);
}

void Tester::select_indices(const QModelIndex first_index, const QModelIndex last_index) {
  auto chord_selection =
      QItemSelection(first_index, last_index);
  editor.selector.select(chord_selection, QItemSelectionModel::Current | QItemSelectionModel::Select);
}

void Tester::unselect_indices(const QModelIndex first_index, const QModelIndex last_index) {
  auto note_selection =
      QItemSelection(first_index, last_index);
  editor.selector.select(note_selection, QItemSelectionModel::Current | QItemSelectionModel::Deselect);
}

void Tester::clear_indices(const QModelIndex first_index, const QModelIndex last_index) {
  auto note_selection =
      QItemSelection(first_index, last_index);
  editor.selector.select(note_selection, QItemSelectionModel::Current | QItemSelectionModel::Clear);
}

void Tester::test_tree() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  auto &root = song.root;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(0);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);
  auto &first_note_node = first_chord_node.get_child(0);

  // test song
  QCOMPARE(song.rowCount(root_index), 2);
  QCOMPARE(song.columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(song.root.get_level(), ROOT_LEVEL);

  // test first chord
  QCOMPARE(first_chord_node.get_level(), CHORD_LEVEL);
  QCOMPARE(song.parent(first_chord_symbol_index), root_index);
  // only nest the symbol column
  QCOMPARE(song.rowCount(song.index(0, numerator_column, root_index)), 0);

  // test first note
  QCOMPARE(song.parent(first_note_symbol_index).row(), 0);
  QCOMPARE(first_note_node.get_level(), NOTE_LEVEL);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row -1");
  first_note_node.assert_child_at(-1);
  QTest::ignoreMessage(QtCriticalMsg, "Only chords can have children!");
  root.new_child_pointer(&first_note_node);
}

void Tester::test_set_value() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto &undo_stack = editor.undo_stack;
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);

  QTest::ignoreMessage(QtCriticalMsg, "No column 0");
  set_data(0, symbol_column, root_index, QVariant());
  QVERIFY(set_data(0, numerator_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, denominator_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, octave_column, root_index, QVariant(1)));
  undo_stack.undo();
  QVERIFY(set_data(0, beats_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, tempo_percent_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, words_column, root_index, QVariant("hello")));
  undo_stack.undo();

  // can't set non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  song.node_from_index(first_chord_symbol_index)
      .note_chord_pointer->setData(-1, QVariant());
  // setData only works for the edit role
  QVERIFY(!(
      song.setData(first_chord_symbol_index, QVariant(), Qt::DecorationRole)));

  QVERIFY(set_data(0, numerator_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(
      set_data(0, denominator_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, octave_column, first_chord_symbol_index, QVariant(1)));
  undo_stack.undo();
  QVERIFY(set_data(0, beats_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, first_chord_symbol_index,
                   QVariant(2)));
  undo_stack.undo();
  QVERIFY(
      set_data(0, tempo_percent_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(
      set_data(0, words_column, first_chord_symbol_index, QVariant("hello")));
  undo_stack.undo();
  QVERIFY(set_data(0, instrument_column, first_chord_symbol_index,
                   QVariant("Wurley")));
  undo_stack.undo();

  // can't set non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  song.node_from_index(first_note_symbol_index)
      .note_chord_pointer->setData(-1, QVariant());
}

void Tester::test_flags() {
  auto &song = editor.song;
  auto &root = song.root;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(0);
  auto &first_note_node = first_chord_node.get_child(0);

  // cant edit the symbol
  QCOMPARE(song.flags(first_chord_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(song.flags(song.index(0, numerator_column, root_index)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  // cant edit the instrument
  QCOMPARE(song.flags(song.index(0, instrument_column, root_index)),
           Qt::NoItemFlags);
  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_chord_node.note_chord_pointer->flags(-1), Qt::NoItemFlags);

  // cant edit the symbol
  QCOMPARE(song.flags(song.index(0, symbol_column, first_chord_symbol_index)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(
      song.flags(song.index(0, numerator_column, first_chord_symbol_index)),
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_note_node.note_chord_pointer->flags(-1), Qt::NoItemFlags);
}

void Tester::test_get_value() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto &first_chord_node = song.root.get_child(0);
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto &first_note_node = first_chord_node.get_child(0);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);

  QCOMPARE(get_data(0, symbol_column, root_index, Qt::DisplayRole),
           QVariant("♫"));
  QCOMPARE(get_data(0, numerator_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_NUMERATOR));
  QCOMPARE(get_data(0, denominator_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_DENOMINATOR));
  QCOMPARE(get_data(0, octave_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_OCTAVE));
  QCOMPARE(get_data(0, beats_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_VOLUME_PERCENT));
  QCOMPARE(get_data(0, tempo_percent_column, root_index, Qt::DisplayRole),
           QVariant(DEFAULT_TEMPO_PERCENT));
  QCOMPARE(get_data(0, words_column, root_index, Qt::DisplayRole),
           QVariant(""));
  QCOMPARE(get_data(0, instrument_column, root_index, Qt::DisplayRole),
           QVariant());

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_chord_node.note_chord_pointer->data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non-display data
  QCOMPARE(song.data(first_chord_symbol_index, Qt::DecorationRole), QVariant());

  QCOMPARE(
      get_data(0, symbol_column, first_chord_symbol_index, Qt::DisplayRole),
      QVariant("♪"));
  QCOMPARE(
      get_data(0, numerator_column, first_chord_symbol_index, Qt::DisplayRole),
      QVariant(DEFAULT_NUMERATOR));
  QCOMPARE(get_data(0, denominator_column, first_chord_symbol_index,
                    Qt::DisplayRole),
           QVariant(DEFAULT_DENOMINATOR));
  QCOMPARE(
      get_data(0, octave_column, first_chord_symbol_index, Qt::DisplayRole),
      QVariant(DEFAULT_OCTAVE));
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index, Qt::DisplayRole),
           QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index,
                    Qt::DisplayRole),
           QVariant(DEFAULT_VOLUME_PERCENT));
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index,
                    Qt::DisplayRole),
           QVariant(DEFAULT_TEMPO_PERCENT));
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index, Qt::DisplayRole),
           QVariant(""));
  QCOMPARE(
      get_data(0, instrument_column, first_chord_symbol_index, Qt::DisplayRole),
      QVariant(DEFAULT_DEFAULT_INSTRUMENT));

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_note_node.note_chord_pointer->data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non display data
  QCOMPARE(song.data(first_note_symbol_index, Qt::DecorationRole), QVariant());
}

void Tester::test_colors() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(0);
  auto &first_note_node = first_chord_node.get_child(0);

  QCOMPARE(get_data(0, symbol_column, root_index, Qt::ForegroundRole), NO_DATA);
  QCOMPARE(get_data(0, numerator_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, denominator_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, octave_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, beats_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, volume_percent_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, tempo_percent_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, words_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, instrument_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_chord_node.note_chord_pointer->data(-1, Qt::ForegroundRole),
           QVariant());

  QCOMPARE(get_data(1, numerator_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, denominator_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, octave_column, root_index, Qt::ForegroundRole), NO_DATA);
  QCOMPARE(get_data(1, beats_column, root_index, Qt::ForegroundRole), NO_DATA);
  QCOMPARE(get_data(1, volume_percent_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, tempo_percent_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, words_column, root_index, Qt::ForegroundRole), NO_DATA);

  QCOMPARE(
      get_data(0, symbol_column, first_chord_symbol_index, Qt::ForegroundRole),
      NO_DATA);
  QCOMPARE(get_data(0, numerator_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, denominator_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(
      get_data(0, octave_column, first_chord_symbol_index, Qt::ForegroundRole),
      LIGHT_GRAY);
  QCOMPARE(
      get_data(0, beats_column, first_chord_symbol_index, Qt::ForegroundRole),
      LIGHT_GRAY);
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(
      get_data(0, words_column, first_chord_symbol_index, Qt::ForegroundRole),
      LIGHT_GRAY);
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           LIGHT_GRAY);

  QCOMPARE(get_data(1, numerator_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, denominator_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(
      get_data(1, octave_column, first_chord_symbol_index, Qt::ForegroundRole),
      NO_DATA);
  QCOMPARE(
      get_data(1, beats_column, first_chord_symbol_index, Qt::ForegroundRole),
      NO_DATA);
  QCOMPARE(get_data(1, volume_percent_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, tempo_percent_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(
      get_data(1, words_column, first_chord_symbol_index, Qt::ForegroundRole),
      NO_DATA);
  QCOMPARE(get_data(1, instrument_column, first_chord_symbol_index,
                    Qt::ForegroundRole),
           NO_DATA);

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_note_node.note_chord_pointer->data(-1, Qt::ForegroundRole),
           QVariant());
}

void Tester::test_orchestra() {
  // test that get_instrument is invalid for chords
  QCOMPARE(editor.song.root.get_child(0).note_chord_pointer->get_instrument(),
           QString());

  auto old_orchestra_text = editor.orchestra_text_edit.toPlainText();
  auto new_orchestra = QString(
      "nchnls = 2\n"
      "0dbfs = 1\n"
      "instr Mandolin2\n"
       "    a_oscilator STKMandolin p4, p5\n"
       "    outs a_oscilator, a_oscilator\n"
       "endin\n"
      "instr Plucked\n"
      "    a_oscilator STKPlucked p4, p5\n"
      "    outs a_oscilator, a_oscilator\n"
      "endin\n"
      "instr Wurley\n"
      "    a_oscilator STKWurley p4, p5\n"
      "    outs a_oscilator, a_oscilator\n"
      "endin\n");
  editor.orchestra_text_edit.setPlainText(new_orchestra);
  editor.save_orchestra_text();
  QCOMPARE(editor.song.orchestra_text, new_orchestra);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.orchestra_text, old_orchestra_text);

  auto no_instrument_orchestra = QString("");
  editor.orchestra_text_edit.setPlainText(no_instrument_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_text, old_orchestra_text);
  editor.orchestra_text_edit.setPlainText(old_orchestra_text);

  auto cannot_parse_orchestra = QString("instr Plucked\ninstr Wurley\nasdf");
  editor.orchestra_text_edit.setPlainText(no_instrument_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_text, old_orchestra_text);
  editor.orchestra_text_edit.setPlainText(old_orchestra_text);

  editor.default_instrument_selector.setCurrentIndex(0);
  editor.save_default_instrument();
  QCOMPARE(editor.song.default_instrument, "Mandolin");
  editor.undo_stack.undo();
  QCOMPARE(editor.song.default_instrument, "Plucked");

  // set default instrument to something that we won't change
  editor.default_instrument_selector.setCurrentIndex(0);
  editor.save_default_instrument();
  // instead, change the instrument of a note
  auto missing_instrument_orchestra = QString(
      "nchnls = 2\n"
      "0dbfs = 1\n"
      "instr Mandolin\n"
       "    a_oscilator STKMandolin p4, p5\n"
       "    outs a_oscilator, a_oscilator\n"
       "endin\n"
      "instr Plucked2\n"
      "    a_oscilator STKPlucked p4, p5\n"
      "    outs a_oscilator, a_oscilator\n"
      "endin\n"
      "instr Wurley\n"
      "    a_oscilator STKWurley p4, p5\n"
      "    outs a_oscilator, a_oscilator\n"
      "endin\n"
      
      );
  editor.orchestra_text_edit.setPlainText(missing_instrument_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_text, old_orchestra_text);
  editor.undo_stack.undo();
  editor.orchestra_text_edit.setPlainText(old_orchestra_text);
}

void Tester::test_sliders() {
  auto old_frequency = editor.frequency_slider.slider.value();
  editor.frequency_slider.slider.setValue(NEW_FREQUENCY);
  editor.set_frequency_with_slider();
  QCOMPARE(editor.song.frequency, NEW_FREQUENCY);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.frequency, old_frequency);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.frequency_slider.slider.value(), NEW_FREQUENCY);
  editor.undo_stack.undo();

  auto old_tempo = editor.tempo_slider.slider.value();
  editor.tempo_slider.slider.setValue(NEW_TEMPO);
  editor.set_tempo_with_slider();
  QCOMPARE(editor.song.tempo, NEW_TEMPO);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.tempo, old_tempo);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.tempo_slider.slider.value(), NEW_TEMPO);
  editor.undo_stack.undo();

  auto old_volume_percent = editor.volume_percent_slider.slider.value();
  editor.volume_percent_slider.slider.setValue(NEW_STARTING_VOLUME_PERCENT);
  editor.set_volume_percent_with_slider();
  QCOMPARE(editor.song.volume_percent, NEW_STARTING_VOLUME_PERCENT);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.volume_percent, old_volume_percent);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.volume_percent_slider.slider.value(),
           NEW_STARTING_VOLUME_PERCENT);
  editor.undo_stack.undo();

  QString const not_an_instrument("Not an instrument");

  QTest::ignoreMessage(QtCriticalMsg,
                       "Cannot find instrument Not an instrument");
  error_instrument(not_an_instrument, false);

  QTest::ignoreMessage(QtCriticalMsg,
                       "Cannot find ComboBox value Not an instrument");
  set_combo_box(editor.default_instrument_selector, not_an_instrument);
}

void Tester::dismiss_save_orchestra_text() {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  editor.save_orchestra_text();
}

void Tester::timer_save_orchestra_text() { editor.save_orchestra_text(); }

void Tester::dismiss_messages() {
  QWidgetList allToplevelWidgets = QApplication::topLevelWidgets();
  foreach (QWidget *w, allToplevelWidgets) {
    if (w->inherits("QMessageBox")) {
      QMessageBox *mb = qobject_cast<QMessageBox *>(w);
      QTest::keyClick(mb, Qt::Key_Enter);
    }
  }
}

void Tester::test_select() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index = song.index(0, symbol_column, root_index);
  auto item_selection =
      QItemSelection(first_chord_symbol_index, first_chord_symbol_index);
  editor.selector.select(item_selection, QItemSelectionModel::Select);
  editor.selector.select(item_selection, QItemSelectionModel::Deselect);
}