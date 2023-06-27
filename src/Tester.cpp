#include "Tester.h"

#include <QtCore/qglobal.h>      // for QtCriticalMsg, QForeachContainer
#include <bits/chrono.h>         // for milliseconds
#include <qabstractitemmodel.h>  // for QModelIndex, QModelIndexList
#include <qaction.h>             // for QAction
#include <qapplication.h>        // for QApplication
#include <qcombobox.h>           // for QComboBox
#include <qitemselectionmodel.h> // for QItemSelectionModel, operator|
#include <qlist.h>               // for QList<>::const_iterator
#include <qmessagebox.h>         // for QMessageBox
#include <qnamespace.h>          // for operator|, DisplayRole, Decoration...
#include <qpointer.h>            // for QPointer
#include <qslider.h>             // for QSlider
#include <qspinbox.h>            // for QSpinBox
#include <qstyleoption.h>        // for QStyleOptionViewItem
#include <qtest.h>               // for qCompare
#include <qtestcase.h>           // for qCompare, QCOMPARE, ignoreMessage
#include <qtestkeyboard.h>       // for keyClick
#include <qtextedit.h>           // for QTextEdit
#include <qtimer.h>              // for QTimer
#include <qtreeview.h>           // for QTreeView
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include <memory>  // for unique_ptr
#include <thread>  // for sleep_for
#include <utility> // for move
#include <vector>  // for vector

#include "ChordsModel.h"          // for ChordsModel, NOTE_CHORD_COLUMNS
#include "ComboBoxItemDelegate.h" // for ComboBoxItemDelegate
#include "Interval.h"              // for Interval, DEFAULT_DENOMINATOR
#include "IntervalDelegate.h"      // for IntervalDelegate
#include "IntervalEditor.h"        // for IntervalEditor
#include "NoteChord.h"            // for symbol_column, numerator_column
#include "ShowSlider.h"           // for ShowSlider
#include "SliderItemDelegate.h"   // for SliderItemDelegate
#include "Song.h"                 // for Song, DEFAULT_STARTING_INSTRUMENT
#include "SpinBoxItemDelegate.h"  // for SpinBoxItemDelegate
#include "TreeNode.h"             // for TreeNode, new_child_pointer
#include "Utilities.h"            // for NON_DEFAULT_COLOR, DEFAULT_COLOR

const auto STARTING_KEY_1 = 401;
const auto STARTING_KEY_2 = 402;
const auto STARTING_TEMPO_1 = 221;
const auto STARTING_TEMPO_2 = 222;
const auto STARTING_VOLUME_1 = 51;
const auto STARTING_VOLUME_2 = 52;

const auto VOLUME_PERCENT_1 = 101;

const auto TWO_DOUBLE = 2.0;

const auto PLAY_WAIT_TIME = 3000;

const auto NO_DATA = QVariant();

const auto MESSAGE_BOX_WAIT = 500;

const auto BIG_ROW = 10;

auto frame_json_chord(const QString &chord_text) -> QString {
  return QString(R""""( {
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50,
    "chords": [%1]
  })"""").arg(chord_text);
}

auto frame_json_note(const QString &chord_text) -> QString {
  return QString(R""""( {
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50,
    "chords": [{"notes": [%1]}]
  })"""").arg(chord_text);
}

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor.song.chords_model_pointer->headerData(column, Qt::Horizontal,
                                                      Qt::DisplayRole);
}

auto Tester::get_data(int row, int column, QModelIndex &parent_index)
    -> QVariant {
  return editor.song.chords_model_pointer->data(
      editor.song.chords_model_pointer->index(row, column, parent_index),
      Qt::DisplayRole);
}

auto Tester::get_color(int row, int column, QModelIndex &parent_index)
    -> QVariant {
  return editor.song.chords_model_pointer->data(
      editor.song.chords_model_pointer->index(row, column, parent_index),
      Qt::ForegroundRole);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) -> bool {
  return editor.song.chords_model_pointer->setData(
      editor.song.chords_model_pointer->index(row, column, parent_index),
      new_value, Qt::EditRole);
}

void Tester::initTestCase() {
  editor.activateWindow();
  editor.load_from(QString(R""""({
    "chords": [
        {
            "notes": [
                {},
                {
                    "interval": "2/2o1",
                    "beats": 2,
                    "volume_percent": 2,
                    "tempo_percent": 2,
                    "words": "hello",
                    "instrument": "Wurley"
                }
            ]
        },
        {
            "interval": "2/2o1",
            "beats": 2,
            "volume_percent": 2.0,
            "tempo_percent": 2.0,
            "words": "hello",
            "instrument": "Wurley",
            "notes": [{}]
        },
        {}
    ],
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50
})"""")
                       .toUtf8());
  // TODO: verify this loaded correctly and if not cancel tests
  first_chord_symbol_index =
      editor.song.chords_model_pointer->index(0, symbol_column, root_index);
  second_chord_symbol_index =
      editor.song.chords_model_pointer->index(1, symbol_column, root_index);
  first_note_instrument_index = editor.song.chords_model_pointer->index(
      0, instrument_column, first_chord_symbol_index);
  first_chord_node_pointer =
      editor.song.chords_model_pointer->root.child_pointers[0].get();
  first_note_node_pointer = first_chord_node_pointer->child_pointers[0].get();
}

void Tester::test_column_headers() {
  // no symbol header
  QCOMPARE(get_column_heading(symbol_column), QVariant());
  QCOMPARE(get_column_heading(interval_column), "Interval");
  QCOMPARE(get_column_heading(beats_column), "Beats");
  QCOMPARE(get_column_heading(volume_percent_column), "Volume %");
  QCOMPARE(get_column_heading(tempo_percent_column), "Tempo %");
  QCOMPARE(get_column_heading(words_column), "Words");
  QCOMPARE(get_column_heading(instrument_column), "Instrument");
  // error for non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(editor.song.chords_model_pointer->headerData(
               interval_column, Qt::Vertical, Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(editor.song.chords_model_pointer->headerData(
               interval_column, Qt::Horizontal, Qt::DecorationRole),
           QVariant());
}

void Tester::test_save() const {
  auto json_document = editor.song.to_json();
  QTest::ignoreMessage(QtCriticalMsg, "Cannot open file not_a_file");
  cannot_open_error("not_a_file");
}

void Tester::test_view() {
  editor.show();
  editor.view_controls_checkbox_pointer->setChecked(false);
  QVERIFY(!(editor.controls_widget_pointer->isVisible()));
  editor.view_controls_checkbox_pointer->setChecked(true);
  QVERIFY(editor.controls_widget_pointer->isVisible());

  editor.view_orchestra_checkbox_pointer->setChecked(false);
  QVERIFY(!(editor.orchestra_box_pointer->isVisible()));
  editor.view_orchestra_checkbox_pointer->setChecked(true);
  QVERIFY(editor.orchestra_box_pointer->isVisible());

  editor.view_chords_checkbox_pointer->setChecked(false);
  QVERIFY(!(editor.chords_view_pointer->isVisible()));
  editor.view_chords_checkbox_pointer->setChecked(true);
  QVERIFY(editor.chords_view_pointer->isVisible());
  editor.close();
}

void Tester::test_insert_delete() {
  auto first_chord_instrument_index =
      editor.song.chords_model_pointer->index(0, instrument_column, root_index);

  auto &second_chord_node =
      *(editor.song.chords_model_pointer->root.child_pointers[1]);

  auto third_chord_symbol_index =
      editor.song.chords_model_pointer->index(2, symbol_column, root_index);
  auto third_chord_instrument_index =
      editor.song.chords_model_pointer->index(2, instrument_column, root_index);
  auto &third_chord_node =
      *(editor.song.chords_model_pointer->root.child_pointers[2]);

  select_index(first_chord_symbol_index);
  editor.copy_selected();
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to copy!");
  editor.copy_selected();

  // paste after first chord
  select_index(first_chord_symbol_index);
  editor.paste_before();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to paste before!");
  editor.paste_before();

  select_index(first_chord_symbol_index);
  editor.paste_after();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to paste after!");
  editor.paste_after();

  select_index(first_chord_symbol_index);
  editor.insert_before();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to insert before!");
  editor.insert_before();

  select_index(first_chord_symbol_index);
  editor.insert_after();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QCOMPARE(editor.chords_view_pointer->selectionModel()->selectedRows().size(),
           0);
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to insert after!");
  editor.insert_after();

  select_index(third_chord_symbol_index);
  editor.insert_into();
  QCOMPARE(third_chord_node.child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(third_chord_node.child_pointers.size(), 0);
  clear_selection();
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor.insert_into();

  select_index(first_chord_symbol_index);
  editor.remove_selected();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 2);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to remove!");
  editor.remove_selected();

  auto first_note_symbol_index = editor.song.chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);

  select_index(first_note_symbol_index);
  editor.copy_selected();
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to copy!");
  editor.copy_selected();

  select_index(first_note_symbol_index);
  editor.paste_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to paste before!");
  editor.paste_before();

  select_index(first_note_symbol_index);
  editor.paste_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to paste after!");
  editor.paste_after();

  select_index(third_chord_symbol_index);
  editor.paste_into();
  QCOMPARE(third_chord_node.child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(third_chord_node.child_pointers.size(), 0);
  clear_selection();

  select_index(first_note_symbol_index);
  editor.insert_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to insert before!");
  editor.insert_before();

  select_index(first_note_symbol_index);
  editor.insert_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to insert after!");
  editor.insert_after();

  select_index(first_note_symbol_index);
  editor.remove_selected();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  QTest::ignoreMessage(QtCriticalMsg, "Nothing to remove!");
  editor.remove_selected();

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 9");
  editor.song.chords_model_pointer->removeRows_no_signal(0, BIG_ROW, root_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 9");
  auto dummy_storage = std::vector<std::unique_ptr<TreeNode>>();
  editor.song.chords_model_pointer->remove_save(0, BIG_ROW, root_index,
                                                dummy_storage);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 10");
  QVERIFY(
      !editor.song.chords_model_pointer->insertRows(BIG_ROW, 1, root_index));

  select_index(first_chord_symbol_index);
  editor.copy_selected();
  clear_selection();

  QTest::ignoreMessage(QtCriticalMsg,
                       "Level mismatch between level 2 and new level 1!");
  editor.song.chords_model_pointer->insert_children(0, editor.copied,
                                                    first_chord_symbol_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 10");
  editor.song.chords_model_pointer->insert_children(BIG_ROW, editor.copied,
                                                    first_chord_symbol_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  auto error_pointer =
      editor.song.chords_model_pointer->root.copy_note_chord_pointer();
}

void Tester::test_play() {
  auto second_chord_instrument_index =
      editor.song.chords_model_pointer->index(1, instrument_column, root_index);

  select_indices(first_chord_symbol_index, second_chord_symbol_index);
  // use the second chord to test key changing
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  select_index(second_chord_symbol_index);
  // use the second chord to test key changing
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  auto second_note_symbol_index = editor.song.chords_model_pointer->index(
      1, symbol_column, first_chord_symbol_index);
  auto second_note_instrument_index = editor.song.chords_model_pointer->index(
      1, symbol_column, first_chord_symbol_index);
  select_index(second_note_symbol_index);
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  auto third_note_symbol_index = editor.song.chords_model_pointer->index(
      0, symbol_column, second_chord_symbol_index);
  auto third_note_instrument_index = editor.song.chords_model_pointer->index(
      0, instrument_column, second_chord_symbol_index);
  select_index(third_note_symbol_index);
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 9");
  editor.play(0, BIG_ROW, root_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.song.chords_model_pointer->root.get_ratio(), -1);

  QTest::ignoreMessage(QtCriticalMsg, "Nothing to play!");
  editor.play_selected();
}

void Tester::select_indices(const QModelIndex first_index,
                            const QModelIndex last_index) {
  auto chord_selection = QItemSelection(first_index, last_index);
  editor.chords_view_pointer->selectionModel()->blockSignals(true);
  editor.chords_view_pointer->selectionModel()->select(
      chord_selection, QItemSelectionModel::Current |
                           QItemSelectionModel::Select |
                           QItemSelectionModel::Rows);
  editor.chords_view_pointer->selectionModel()->blockSignals(false);
}

void Tester::select_index(const QModelIndex index) {
  select_indices(index, index);
}

void Tester::clear_selection() {
  editor.chords_view_pointer->selectionModel()->select(
      QItemSelection(),
      QItemSelectionModel::Current | QItemSelectionModel::Clear);
}

void Tester::test_tree() {
  auto &root = editor.song.chords_model_pointer->root;

  TreeNode untethered(editor.song.instrument_pointers, &root);
  QTest::ignoreMessage(QtCriticalMsg, "Not a child!");
  QCOMPARE(untethered.is_at_row(), -1);

  auto first_note_symbol_index = editor.song.chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);

  // test song
  QCOMPARE(editor.song.chords_model_pointer->rowCount(root_index), 3);
  QCOMPARE(editor.song.chords_model_pointer->columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(editor.song.chords_model_pointer->root.get_level(), root_level);

  // test first chord
  QCOMPARE(first_chord_node_pointer->get_level(), chord_level);
  QCOMPARE(editor.song.chords_model_pointer->parent(first_chord_symbol_index),
           root_index);
  // only nest the symbol column
  QCOMPARE(editor.song.chords_model_pointer->rowCount(
               editor.song.chords_model_pointer->index(0, interval_column,
                                                       root_index)),
           0);

  // test first note
  QCOMPARE(
      editor.song.chords_model_pointer->parent(first_note_symbol_index).row(),
      0);
  QCOMPARE(first_note_node_pointer->get_level(), note_level);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row -1");
  QVERIFY(!(first_note_node_pointer->verify_child_at(-1)));
  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 2!");
  new_child_pointer(first_note_node_pointer);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.song.chords_model_pointer->parent(root_index), QModelIndex());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.song.chords_model_pointer->root.is_at_row(), -1);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid row 10");
  QCOMPARE(editor.song.chords_model_pointer->index(BIG_ROW, symbol_column,
                                                   root_index),
           root_index);
}

// TODO: better actually changed
void Tester::test_set_value() {
  auto first_note_symbol_index = editor.song.chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);

  QTest::ignoreMessage(QtCriticalMsg, "No column 0");
  QVERIFY(set_data(0, symbol_column, root_index, QVariant()));
  
  QVERIFY(set_data(0, interval_column, root_index, QVariant("2")));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, beats_column, root_index, QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, root_index, QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, tempo_percent_column, root_index, QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, words_column, root_index, QVariant("hello")));
  editor.undo_stack.undo();

  // can't set non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QVERIFY(!(editor.song.chords_model_pointer
                ->node_from_index(first_chord_symbol_index)
                .note_chord_pointer->setData(-1, QVariant())));
  // setData only works for the edit role
  QVERIFY(!(editor.song.chords_model_pointer->setData(
      first_chord_symbol_index, QVariant(), Qt::DecorationRole)));

  QVERIFY(set_data(0, interval_column, first_chord_symbol_index, QVariant("2")));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, beats_column, first_chord_symbol_index, QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, first_chord_symbol_index,
                   QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(
      set_data(0, tempo_percent_column, first_chord_symbol_index, QVariant(2)));
  editor.undo_stack.undo();
  QVERIFY(
      set_data(0, words_column, first_chord_symbol_index, QVariant("hello")));
  editor.undo_stack.undo();
  QVERIFY(set_data(0, instrument_column, first_chord_symbol_index,
                   QVariant("Wurley")));
  editor.undo_stack.undo();

  // can't set non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QVERIFY(!(
      editor.song.chords_model_pointer->node_from_index(first_note_symbol_index)
          .note_chord_pointer->setData(-1, QVariant())));

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  editor.song.chords_model_pointer->setData_irreversible(root_index, QVariant());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  editor.song.chords_model_pointer->setData_irreversible(root_index, QVariant());
}

void Tester::test_flags() {
  auto &root = editor.song.chords_model_pointer->root;

  // cant edit the symbol
  QCOMPARE(editor.song.chords_model_pointer->flags(first_chord_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(editor.song.chords_model_pointer->flags(
               editor.song.chords_model_pointer->index(0, interval_column,
                                                       root_index)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(editor.song.chords_model_pointer->column_flags(-1), Qt::NoItemFlags);
  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.song.chords_model_pointer->flags(root_index),
           Qt::NoItemFlags);
}

void Tester::test_get_value() {
  auto first_note_symbol_index = editor.song.chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);

  QCOMPARE(get_data(0, symbol_column, root_index), QVariant("♫"));
  QCOMPARE(get_data(0, interval_column, root_index), QVariant("1"));
  QCOMPARE(get_data(0, beats_column, root_index), QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, root_index), QVariant(100));
  QCOMPARE(get_data(0, tempo_percent_column, root_index), QVariant(100));
  QCOMPARE(get_data(0, words_column, root_index), QVariant(DEFAULT_WORDS));
  QCOMPARE(get_data(0, instrument_column, root_index), QVariant(""));

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_chord_node_pointer->note_chord_pointer->data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non-display data
  QCOMPARE(editor.song.chords_model_pointer->data(first_chord_symbol_index,
                                                  Qt::DecorationRole),
           QVariant());

  QCOMPARE(get_data(0, symbol_column, first_chord_symbol_index), QVariant("♪"));
  QCOMPARE(get_data(0, interval_column, first_chord_symbol_index),
           QVariant("1"));
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index),
           QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index),
           QVariant(100));
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index),
           QVariant(100));
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index),
           QVariant(DEFAULT_WORDS));
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
           QVariant(""));

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_note_node_pointer->note_chord_pointer->data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non display data
  QCOMPARE(editor.song.chords_model_pointer->data(first_note_symbol_index,
                                                  Qt::DecorationRole),
           QVariant());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.song.chords_model_pointer->data(root_index, symbol_column),
           QVariant());

  auto test_interval = Interval();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.get_text(), "1/2");
  test_interval.denominator = DEFAULT_DENOMINATOR;
  test_interval.octave = 1;
  QCOMPARE(test_interval.get_text(), "1o1");
}

void Tester::test_json() {
  QVERIFY(!(dismiss_load_text("{")));
  QVERIFY(!dismiss_load_text("[]"));
  QVERIFY(!dismiss_load_text("{}"));
  // missing field
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50,
    "not a field": 1
  })""""));
  // non-parsing orchestra
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\nasdf",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-string orchestra
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": 1,
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-string starting instrument
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": 1,
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-existent starting instrument
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Not an instrument",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-double starting key
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": "",
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // below minimum starting key
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": -1,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-double starting tempo
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": "",
    "starting_volume": 50
  })""""));
  // below minimum starting tempo
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": -1,
    "starting_volume": 50
  })""""));
  // non-double starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": ""
  })""""));
  // negative starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": -1
  })""""));
  // above maximum starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 101
  })""""));
  // non-array chords
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Plucked",
    "starting_key": 220,
    "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "starting_tempo": 200,
    "starting_volume": 50,
    "chords": 1
  })""""));
  QVERIFY(!dismiss_load_text(frame_json_chord("1")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"not a field\": 1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"1/0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"1/200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"1o-20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"interval\": \"1o20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"beats\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"beats\": 1.5}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"beats\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"beats\": 100}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"words\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"instrument\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"instrument\": \"not an instrument\"}")));
  QVERIFY(!dismiss_load_text(R""""(
    {
      "starting_instrument": "Plucked",
      "starting_key": 220,
      "orchestra_code": "nchnls = 2\n0dbfs = 1\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
      "starting_tempo": 200,
      "starting_volume": 50,
      "chords": [
        {
          "notes": -1
        }
      ]
    }
  )""""));
  QVERIFY(!dismiss_load_text(frame_json_note("1")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"not a field\": 1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"not an interval\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1/0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1/200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1o-20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1o20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": 1.5}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": 100}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"words\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"instrument\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"instrument\": \"not an instrument\"}")));
}

void Tester::test_colors() {

  QCOMPARE(get_color(0, symbol_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(0, interval_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, beats_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, volume_percent_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, tempo_percent_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, words_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, instrument_column, root_index), DEFAULT_COLOR);
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_chord_node_pointer->note_chord_pointer->data(-1, Qt::ForegroundRole),
           QVariant());

  QCOMPARE(get_color(1, interval_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, beats_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, volume_percent_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, tempo_percent_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, words_column, root_index), NON_DEFAULT_COLOR);

  QCOMPARE(get_color(0, symbol_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(0, interval_column, first_chord_symbol_index),
           DEFAULT_COLOR);
  QCOMPARE(get_color(0, beats_column, first_chord_symbol_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, volume_percent_column, first_chord_symbol_index),
           DEFAULT_COLOR);
  QCOMPARE(get_color(0, tempo_percent_column, first_chord_symbol_index),
           DEFAULT_COLOR);
  QCOMPARE(get_color(0, words_column, first_chord_symbol_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, instrument_column, first_chord_symbol_index),
           DEFAULT_COLOR);

  QCOMPARE(get_color(1, interval_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, beats_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, volume_percent_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, tempo_percent_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, words_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, instrument_column, first_chord_symbol_index),
           NON_DEFAULT_COLOR);

  // error on non-existent column
  QTest::ignoreMessage(QtCriticalMsg, "No column -1");
  QCOMPARE(first_note_node_pointer->note_chord_pointer->data(-1, Qt::ForegroundRole),
           QVariant());
}

void Tester::test_orchestra() {
  QTest::ignoreMessage(QtCriticalMsg,
                       "Cannot find starting instrument not an instrument");
  Song broken_song_1(editor.csound_session, editor.undo_stack,
                     "not an instrument");
  QTest::ignoreMessage(QtCriticalMsg,
                       "Cannot compile orchestra, error code -1");
  Editor broken_editor("Plucked", "instr Plucked asdf");

  // test a valid orchestra change
  auto old_orchestra_text = editor.orchestra_editor_pointer->toPlainText();
  auto new_orchestra = QString("nchnls = 2\n"
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
  editor.orchestra_editor_pointer->setPlainText(new_orchestra);
  editor.save_orchestra_code();
  QCOMPARE(editor.song.orchestra_code, new_orchestra);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.orchestra_code, old_orchestra_text);

  // test empty orchestra
  auto empty_orchestra = QString("");
  editor.orchestra_editor_pointer->setPlainText(empty_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_code, old_orchestra_text);
  editor.orchestra_editor_pointer->setPlainText(old_orchestra_text);

  // test non-parsable orchestra
  auto cannot_parse_orchestra =
      QString("instr Mandolin\ninstr Plucked\ninstr Wurley\nasdf");
  editor.orchestra_editor_pointer->setPlainText(cannot_parse_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_code, old_orchestra_text);
  editor.orchestra_editor_pointer->setPlainText(old_orchestra_text);

  // test default instrument change
  editor.starting_instrument_selector_pointer->setCurrentText("Mandolin");
  editor.save_starting_instrument();
  QCOMPARE(editor.song.starting_instrument, "Mandolin");
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_instrument, "Plucked");

  // test missing instrument change
  // set default instrument to something that we won't change
  editor.starting_instrument_selector_pointer->setCurrentText("Mandolin");
  editor.save_starting_instrument();
  // instead, change the instrument of a note
  auto missing_instrument_orchestra =
      QString("nchnls = 2\n"
              "0dbfs = 1\n"
              "instr Mandolin\n"
              "    a_oscilator STKMandolin p4, p5\n"
              "    outs a_oscilator, a_oscilator\n"
              "endin\n"
              "instr Plucked\n"
              "    a_oscilator STKPlucked p4, p5\n"
              "    outs a_oscilator, a_oscilator\n"
              "endin\n"
              "instr Wurley2\n"
              "    a_oscilator STKWurley p4, p5\n"
              "    outs a_oscilator, a_oscilator\n"
              "endin\n"

      );
  editor.orchestra_editor_pointer->setPlainText(missing_instrument_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_code, old_orchestra_text);
  editor.undo_stack.undo();
  editor.orchestra_editor_pointer->setPlainText(old_orchestra_text);

  // set default instrument mismatch
  editor.starting_instrument_selector_pointer->setCurrentText("Mandolin");
  editor.save_starting_instrument();
  QCOMPARE(editor.song.starting_instrument, "Mandolin");
  // change default instrument
  auto default_mismatch_orchestra =
      QString("nchnls = 2\n"
              "0dbfs = 1\n"
              "instr BandedWG\n"
              "    a_oscilator STKBandedWG p4, p5\n"
              "    outs a_oscilator, a_oscilator\n"
              "endin\n"
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
  editor.orchestra_editor_pointer->setPlainText(default_mismatch_orchestra);
  dismiss_save_orchestra_text();
  QCOMPARE(editor.song.orchestra_code, default_mismatch_orchestra);
  QCOMPARE(editor.song.starting_instrument, "BandedWG");
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_instrument, "Mandolin");
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_instrument, "Plucked");
  editor.orchestra_editor_pointer->setPlainText(old_orchestra_text);
}

void Tester::test_sliders() {
  auto old_frequency =
      editor.starting_key_slider_pointer->slider_pointer->value();
  editor.starting_key_slider_pointer->slider_pointer->setValue(STARTING_KEY_1);
  QCOMPARE(editor.song.starting_key, STARTING_KEY_1);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_key, old_frequency);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_key_slider_pointer->slider_pointer->value(),
           STARTING_KEY_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_key_slider_pointer->slider_pointer->setValue(STARTING_KEY_1);
  editor.starting_key_slider_pointer->slider_pointer->setValue(STARTING_KEY_2);
  QCOMPARE(editor.song.starting_key, STARTING_KEY_2);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_key, old_frequency);

  auto old_tempo =
      editor.starting_tempo_slider_pointer->slider_pointer->value();
  editor.starting_tempo_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  QCOMPARE(editor.song.starting_tempo, STARTING_TEMPO_1);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_tempo, old_tempo);

  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_tempo_slider_pointer->slider_pointer->value(),
           STARTING_TEMPO_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_tempo_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  editor.starting_tempo_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_2);
  QCOMPARE(editor.song.starting_tempo, STARTING_TEMPO_2);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_tempo, old_tempo);

  auto old_volume_percent =
      editor.starting_volume_slider_pointer->slider_pointer->value();
  editor.starting_volume_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  QCOMPARE(editor.song.starting_volume, STARTING_VOLUME_1);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_volume, old_volume_percent);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_volume_slider_pointer->slider_pointer->value(),
           STARTING_VOLUME_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_volume_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  editor.starting_volume_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_2);
  QCOMPARE(editor.song.starting_volume, STARTING_VOLUME_2);
  editor.undo_stack.undo();
  QCOMPARE(editor.song.starting_volume, old_volume_percent);

  QString const not_an_instrument("Not an instrument");

  QTest::ignoreMessage(QtCriticalMsg,
                       "Cannot find ComboBox value Not an instrument");
  set_combo_box(*(editor.starting_instrument_selector_pointer),
                not_an_instrument);
}

void Tester::dismiss_save_orchestra_text() {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  editor.save_orchestra_code();
}

auto Tester::dismiss_load_text(const QString &text) -> bool {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  return editor.song.load_from(text.toUtf8());
}

void Tester::dismiss_messages() {
  foreach (QWidget *window_pointer, QApplication::topLevelWidgets()) {
    if (window_pointer->inherits("QMessageBox")) {
      QTest::keyClick(qobject_cast<QMessageBox *>(window_pointer),
                      Qt::Key_Enter);
    }
  }
}

void Tester::test_select() {
  auto item_selection =
      QItemSelection(first_chord_symbol_index, second_chord_symbol_index);
  editor.chords_view_pointer->selectionModel()->select(
      item_selection, QItemSelectionModel::Select);
  editor.chords_view_pointer->selectionModel()->select(
      item_selection, QItemSelectionModel::Deselect);
}

void Tester::test_delegates() {
  auto first_chord_interval_index = editor.song.chords_model_pointer->index(0, interval_column, root_index);
  auto first_chord_beats_index =
      editor.song.chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index =
      editor.song.chords_model_pointer->index(0, volume_percent_column,
                                              root_index);
                                            
  std::unique_ptr<IntervalEditor> interval_editor_pointer(
      dynamic_cast<IntervalEditor *>(editor.interval_delegate_pointer->createEditor(
          nullptr, QStyleOptionViewItem(), first_chord_interval_index)));
  
  editor.interval_delegate_pointer->updateEditorGeometry(
    interval_editor_pointer.get(),
    QStyleOptionViewItem(),
    first_chord_interval_index
  );

  QCOMPARE(interval_editor_pointer->size(), interval_editor_pointer->sizeHint());
  
  editor.interval_delegate_pointer->setEditorData(
    interval_editor_pointer.get(),
    first_chord_interval_index
  );

  QCOMPARE(interval_editor_pointer->numerator_box_pointer->value(), 1);

  interval_editor_pointer->numerator_box_pointer->setValue(2);

  editor.interval_delegate_pointer->setModelData(
      interval_editor_pointer.get(), editor.song.chords_model_pointer,
      first_chord_interval_index);

  QCOMPARE(get_data(0, interval_column, root_index), QVariant("2"));

  editor.undo_stack.undo();

  QCOMPARE(get_data(0, interval_column, root_index), QVariant("1"));

  std::unique_ptr<QSpinBox> spin_box_pointer(
      dynamic_cast<QSpinBox *>(editor.beats_delegate_pointer->createEditor(
          nullptr, QStyleOptionViewItem(), first_chord_beats_index)));
  
  editor.beats_delegate_pointer->updateEditorGeometry(
    spin_box_pointer.get(),
    QStyleOptionViewItem(),
    first_chord_beats_index
  );

  QCOMPARE(spin_box_pointer->size(), spin_box_pointer->sizeHint());
  
  editor.beats_delegate_pointer->setEditorData(
    spin_box_pointer.get(),
    first_chord_beats_index
  );

  QCOMPARE(spin_box_pointer->value(), 1);

  spin_box_pointer->setValue(2);

  editor.beats_delegate_pointer->setModelData(
      spin_box_pointer.get(), editor.song.chords_model_pointer,
      first_chord_beats_index);

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(2));

  editor.undo_stack.undo();

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(1));

  std::unique_ptr<ShowSlider> show_slider_pointer(
      dynamic_cast<ShowSlider *>(
          editor.volume_percent_delegate_pointer->createEditor(
              nullptr, QStyleOptionViewItem(),
              first_chord_volume_percent_index)));
  
  editor.volume_percent_delegate_pointer->updateEditorGeometry(
    show_slider_pointer.get(),
    QStyleOptionViewItem(),
    first_chord_volume_percent_index
  );

  QCOMPARE(show_slider_pointer->size(), show_slider_pointer->sizeHint());


  editor.volume_percent_delegate_pointer->setEditorData(
    show_slider_pointer.get(),
    first_chord_volume_percent_index
  );

  QCOMPARE(show_slider_pointer->slider_pointer->value(), 100);

  show_slider_pointer->set_value_override(VOLUME_PERCENT_1);

  editor.volume_percent_delegate_pointer->setModelData(
      show_slider_pointer.get(), editor.song.chords_model_pointer,
      first_chord_volume_percent_index);

  QCOMPARE(get_data(0, volume_percent_column, root_index), QVariant(101));

  editor.undo_stack.undo();
  
  QCOMPARE(get_data(0, volume_percent_column, root_index), QVariant(100));

  std::unique_ptr<QComboBox> combo_box_delegate_pointer(
      dynamic_cast<QComboBox *>(
          editor.instrument_delegate_pointer->createEditor(
              nullptr, QStyleOptionViewItem(), first_note_instrument_index)));
  
  editor.instrument_delegate_pointer->updateEditorGeometry(
    combo_box_delegate_pointer.get(),
    QStyleOptionViewItem(),
    first_note_instrument_index
  );

  QCOMPARE(combo_box_delegate_pointer->size(), combo_box_delegate_pointer->sizeHint());

  editor.instrument_delegate_pointer->setEditorData(
    combo_box_delegate_pointer.get(),
    first_note_instrument_index
  );

  QCOMPARE(combo_box_delegate_pointer -> currentText(), "Plucked");

  set_combo_box(*combo_box_delegate_pointer, "Wurley");

  editor.instrument_delegate_pointer->setModelData(
      combo_box_delegate_pointer.get(), editor.song.chords_model_pointer,
      first_note_instrument_index);

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
           QVariant("Wurley"));

  editor.undo_stack.undo();

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
           QVariant("Plucked"));
}