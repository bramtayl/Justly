#include "Tester.h"

#include <QtCore/qglobal.h>       // for QtCriticalMsg, QForeachContainer
#include <qabstractitemmodel.h>   // for QModelIndex, QModelIndexList
#include <qaction.h>              // for QAction
#include <qapplication.h>         // for QApplication
#include <qcombobox.h>            // for QComboBox
#include <qitemselectionmodel.h>  // for QItemSelectionModel, operator|, QIt...
#include <qjsonarray.h>
#include <qlist.h>          // for QList<>::const_iterator
#include <qmessagebox.h>    // for QMessageBox
#include <qnamespace.h>     // for operator|, DisplayRole, DecorationRole
#include <qpointer.h>       // for QPointer
#include <qslider.h>        // for QSlider
#include <qspinbox.h>       // for QSpinBox
#include <qstyleoption.h>   // for QStyleOptionViewItem
#include <qtemporaryfile.h>
#include <qtest.h>          // for qCompare
#include <qtestcase.h>      // for qCompare, QCOMPARE, ignoreMessage
#include <qtestkeyboard.h>  // for keyClick
#include <qtimer.h>         // for QTimer
#include <qtreeview.h>      // for QTreeView
#include <qundostack.h>     // for QUndoStack
#include <qvariant.h>       // for QVariant, qvariant_cast
#include <qwidget.h>        // for QWidget

#include <algorithm>
#include <chrono>
#include <memory>   // for unique_ptr, allocator_traits<>::val...
#include <thread>   // for sleep_for
#include <utility>  // for move
#include <vector>   // for vector

#include "ChordsModel.h"         // for ChordsModel
#include "ComboBoxDelegate.h"    // for ComboBoxDelegate
#include "Interval.h"            // for Interval, DEFAULT_DENOMINATOR
#include "IntervalDelegate.h"    // for IntervalDelegate
#include "IntervalEditor.h"      // for IntervalEditor
#include "NoteChord.h"           // for symbol_column, interval_column, ins...
#include "ShowSlider.h"          // for ShowSlider
#include "ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "Song.h"                // for Song
#include "SpinBoxDelegate.h"     // for SpinBoxDelegate
#include "SuffixedNumber.h"      // for SuffixedNumber
#include "TreeNode.h"            // for TreeNode, new_child_pointer
#include "Editor.h"          // for Editor

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

Tester::Tester() {
};

auto frame_json_chord(const QString &chord_text) -> QString {
  return QString(R""""( {
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50,
    "chords": [%1]
  })"""")
      .arg(chord_text);
}

auto frame_json_note(const QString &chord_text) -> QString {
  return QString(R""""( {
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50,
    "chords": [{"notes": [%1]}]
  })"""")
      .arg(chord_text);
}

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor.chords_model_pointer->headerData(column, Qt::Horizontal,
                                                 Qt::DisplayRole);
}

auto Tester::get_data(int row, int column, QModelIndex &parent_index)
    -> QVariant {
  return editor.chords_model_pointer->data(
      editor.chords_model_pointer->index(row, column, parent_index),
      Qt::DisplayRole);
}

auto Tester::get_color(int row, int column, QModelIndex &parent_index)
    -> QVariant {
  return editor.chords_model_pointer->data(
      editor.chords_model_pointer->index(row, column, parent_index),
      Qt::ForegroundRole);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) -> bool {
  return editor.chords_model_pointer->setData(
      editor.chords_model_pointer->index(row, column, parent_index), new_value,
      Qt::EditRole);
}

void Tester::initTestCase() {
    if (main_file.open()) {
      main_file.write(R""""({
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
                    "instrument": "Oboe"
                }
            ]
        },
        {
            "interval": "2/2o1",
            "beats": 2,
            "volume_percent": 2.0,
            "tempo_percent": 2.0,
            "words": "hello",
            "instrument": "Oboe",
            "notes": [{}]
        },
        {}
    ],
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50
})"""");
    main_file.close();
  }
  editor.open_file(main_file.fileName());

  QCOMPARE(song.root.number_of_children(), 3);
  first_chord_node_pointer =
      editor.chords_model_pointer->root.child_pointers[0].get();
  QCOMPARE(first_chord_node_pointer->number_of_children(), 2);
  QCOMPARE(song.root.child_pointers[1]->number_of_children(), 1);
  QVERIFY(player.set_up_correctly);

  first_note_node_pointer = first_chord_node_pointer->child_pointers[0].get();
  third_chord_node_pointer =
      editor.chords_model_pointer->root.child_pointers[2].get();

  first_chord_symbol_index =
      editor.chords_model_pointer->index(0, symbol_column, root_index);
  second_chord_symbol_index =
      editor.chords_model_pointer->index(1, symbol_column, root_index);
  first_note_instrument_index = editor.chords_model_pointer->index(
      0, instrument_column, first_chord_symbol_index);

  first_note_symbol_index = editor.chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);
  third_chord_symbol_index =
      editor.chords_model_pointer->index(2, symbol_column, root_index);
}

void Tester::test_column_headers() {
  // no symbol header
  QCOMPARE(get_column_heading(symbol_column), QVariant());
  QCOMPARE(get_column_heading(interval_column), "Interval");
  QCOMPARE(get_column_heading(beats_column), "Beats");
  QCOMPARE(get_column_heading(volume_percent_column), "Volume");
  QCOMPARE(get_column_heading(tempo_percent_column), "Tempo");
  QCOMPARE(get_column_heading(words_column), "Words");
  QCOMPARE(get_column_heading(instrument_column), "Instrument");
  // error for non-existent column
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(editor.chords_model_pointer->headerData(
                interval_column, Qt::Vertical, Qt::DisplayRole),
            QVariant());
  // headers only for display role
  QCOMPARE(editor.chords_model_pointer->headerData(
                interval_column, Qt::Horizontal, Qt::DecorationRole),
            QVariant());
}

void Tester::test_view() {
  editor.show();
  editor.view_controls_checkbox_pointer->setChecked(false);
  QVERIFY(!(editor.controls_pointer->isVisible()));
  editor.view_controls_checkbox_pointer->setChecked(true);
  QVERIFY(editor.controls_pointer->isVisible());
  editor.close();
}

void Tester::test_insert_delete() {
  select_index(first_chord_symbol_index);
  editor.insert_before();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  editor.insert_before();

  select_index(first_chord_symbol_index);
  editor.insert_after();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  QCOMPARE(
      editor.chords_view_pointer->selectionModel()->selectedRows().size(), 0);
  editor.insert_after();

  select_index(third_chord_symbol_index);
  editor.insert_into();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 0);
  clear_selection();
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor.insert_into();

  select_index(first_chord_symbol_index);
  editor.remove_selected();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 2);
  editor.undo_stack.undo();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  editor.remove_selected();

  select_index(first_note_symbol_index);
  editor.insert_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor.insert_before();

  select_index(first_note_symbol_index);
  editor.insert_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor.insert_after();

  select_index(first_note_symbol_index);
  editor.remove_selected();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor.remove_selected();

  QTest::ignoreMessage(QtCriticalMsg, "No child at index 9!");
  song.root.remove_children(0, BIG_ROW);

  QTest::ignoreMessage(QtCriticalMsg, "No child at index 9!");
  auto dummy_storage = std::vector<std::unique_ptr<TreeNode>>();
  editor.chords_model_pointer->remove_save(0, BIG_ROW, root_index,
                                            dummy_storage);

  QTest::ignoreMessage(QtCriticalMsg, "Can't insert child at index 10!");
  QVERIFY(editor.chords_model_pointer->insertRows(BIG_ROW, 1, root_index));
}

void Tester::test_copy_paste() {
  select_index(first_chord_symbol_index);
  editor.copy_selected();
  clear_selection();
  editor.copy_selected();

  // paste after first chord
  select_index(first_chord_symbol_index);
  editor.paste_before();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  editor.paste_before();

  select_index(first_chord_symbol_index);
  editor.paste_after();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 4);
  editor.undo_stack.undo();
  QCOMPARE(editor.chords_model_pointer->root.child_pointers.size(), 3);
  clear_selection();
  editor.paste_after();

  select_index(first_note_symbol_index);
  editor.copy_selected();
  clear_selection();
  editor.copy_selected();

  select_index(first_note_symbol_index);
  editor.paste_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor.paste_before();

  select_index(first_note_symbol_index);
  editor.paste_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor.undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor.paste_after();

  select_index(third_chord_symbol_index);
  editor.paste_into();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 1);
  editor.undo_stack.undo();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 0);
  clear_selection();

  dismiss_paste(0, "[", root_index);
  dismiss_paste(0, "{}", root_index);
  dismiss_paste(0, "[{\"not a field\": 1}]", root_index);
  dismiss_paste(0, "[{\"not a field\": 1}]", first_chord_symbol_index);

  QTest::ignoreMessage(QtCriticalMsg, "No child at index 10!");
  QCOMPARE(song.root.copy_json_children(BIG_ROW, 1).size(), 0);
  QTest::ignoreMessage(QtCriticalMsg, "No child at index 9!");
  QCOMPARE(song.root.copy_json_children(0, BIG_ROW).size(), 0);

  std::vector<std::unique_ptr<TreeNode>> insertion;
  insertion.push_back(
      std::move(std::make_unique<TreeNode>(first_chord_node_pointer)));
  QTest::ignoreMessage(QtCriticalMsg,
                        "Parent with level 0 cannot contain children with level 2!");
  song.root.insert_children(0, insertion);
}

void Tester::test_play() {
  QTest::ignoreMessage(QtCriticalMsg,
                        "Cannot find starting instrument \"not an instrument\"!");
  Song broken_song_1("not an instrument");

  QTest::ignoreMessage(
      QtCriticalMsg,
      "Cannot find instrument \"not an instrument\"!");
  QCOMPARE(-1, song.get_instrument_id("not an instrument"));

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

  select_index(editor.chords_model_pointer->index(1, symbol_column,
                                                  first_chord_symbol_index));
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  select_index(editor.chords_model_pointer->index(0, symbol_column,
                                                  second_chord_symbol_index));
  editor.play_selected();
  // first cut off early
  editor.play_selected();
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(PLAY_WAIT_TIME));
  clear_selection();

  QTest::ignoreMessage(QtCriticalMsg, "No child at index 9!");
  editor.play(0, BIG_ROW, root_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.chords_model_pointer->root.get_ratio(), -1);

  editor.play_selected();

  editor.stop_playing();
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
  auto &root = editor.chords_model_pointer->root;

  TreeNode untethered(&root);
  QTest::ignoreMessage(QtCriticalMsg, "Not a child!");
  QCOMPARE(untethered.get_row(), -1);
  // test song
  QCOMPARE(editor.chords_model_pointer->rowCount(root_index), 3);
  QCOMPARE(editor.chords_model_pointer->columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(editor.chords_model_pointer->root.get_level(), root_level);

  // test first chord
  QCOMPARE(first_chord_node_pointer->get_level(), chord_level);
  QCOMPARE(editor.chords_model_pointer->parent(first_chord_symbol_index),
            root_index);
  // only nest the symbol column
  QCOMPARE(
      editor.chords_model_pointer->rowCount(
          editor.chords_model_pointer->index(0, interval_column, root_index)),
      0);

  // test first note
  QCOMPARE(editor.chords_model_pointer->parent(first_note_symbol_index).row(),
            0);
  QCOMPARE(first_note_node_pointer->get_level(), note_level);

  QTest::ignoreMessage(QtCriticalMsg, "No child at index -1!");
  QVERIFY(!(first_note_node_pointer->verify_child_at(-1)));
  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 2!");
  new_child_pointer(first_note_node_pointer);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.chords_model_pointer->parent(root_index), QModelIndex());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.chords_model_pointer->root.get_row(), -1);

  QTest::ignoreMessage(QtCriticalMsg, "No child at index 10!");
  QCOMPARE(
      editor.chords_model_pointer->index(BIG_ROW, symbol_column, root_index),
      root_index);

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 2!");
  QVERIFY(!first_note_node_pointer->verify_json_children(song,
                                                          QJsonArray()));

  QTest::ignoreMessage(QtCriticalMsg, "Can't insert child at index -1!");
  song.root.insert_json_children(-1, QJsonArray());

  QTest::ignoreMessage(QtCriticalMsg, "Can't insert child at index -1!");
  song.root.insert_children(-1, song.root.child_pointers);
}

void Tester::test_set_value() {
  QVERIFY(set_data(0, symbol_column, root_index, QVariant()));

  QVERIFY(set_data(0, interval_column, root_index,
                    QVariant::fromValue(Interval(2))));
  QCOMPARE(get_data(0, interval_column, root_index),
            QVariant::fromValue(Interval(2)));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, interval_column, root_index),
            QVariant::fromValue(Interval()));

  QVERIFY(set_data(0, beats_column, root_index, QVariant(2)));
  QCOMPARE(get_data(0, beats_column, root_index), QVariant(2));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, beats_column, root_index), QVariant(1));

  QVERIFY(set_data(0, volume_percent_column, root_index,
                    QVariant::fromValue(SuffixedNumber(2, "%"))));
  QCOMPARE(get_data(0, volume_percent_column, root_index),
            QVariant::fromValue(SuffixedNumber(2, "%")));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, volume_percent_column, root_index),
            QVariant::fromValue(SuffixedNumber(100, "%")));

  QVERIFY(set_data(0, tempo_percent_column, root_index,
                    QVariant::fromValue(SuffixedNumber(2, "%"))));
  QCOMPARE(get_data(0, tempo_percent_column, root_index),
            QVariant::fromValue(SuffixedNumber(2, "%")));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, tempo_percent_column, root_index),
            QVariant::fromValue(SuffixedNumber(100, "%")));

  QVERIFY(set_data(0, words_column, root_index, QVariant("hello")));
  QCOMPARE(get_data(0, words_column, root_index), QVariant("hello"));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, words_column, root_index), QVariant(""));

  QVERIFY(set_data(0, instrument_column, root_index, QVariant("Oboe")));
  QCOMPARE(get_data(0, instrument_column, root_index), QVariant("Oboe"));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, instrument_column, root_index), QVariant(""));

  editor.chords_model_pointer->get_node(first_chord_symbol_index)
      .note_chord_pointer->setData(-1, QVariant());
  // setData only works for the edit role
  QVERIFY(!(editor.chords_model_pointer->setData(
      first_chord_symbol_index, QVariant(), Qt::DecorationRole)));

  QVERIFY(set_data(0, interval_column, first_chord_symbol_index,
                    QVariant::fromValue(Interval(2))));
  QCOMPARE(get_data(0, interval_column, first_chord_symbol_index),
            QVariant::fromValue(Interval(2)));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, interval_column, first_chord_symbol_index),
            QVariant::fromValue(Interval()));

  QVERIFY(set_data(0, beats_column, first_chord_symbol_index, QVariant(2)));
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index), QVariant(2));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index), QVariant(1));

  QVERIFY(set_data(0, volume_percent_column, first_chord_symbol_index,
                    QVariant::fromValue(SuffixedNumber(2, "%"))));
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index),
            QVariant::fromValue(SuffixedNumber(2, "%")));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index),
            QVariant::fromValue(SuffixedNumber(100, "%")));

  QVERIFY(set_data(0, tempo_percent_column, first_chord_symbol_index,
                    QVariant::fromValue(SuffixedNumber(2, "%"))));
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index),
            QVariant::fromValue(SuffixedNumber(2, "%")));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index),
            QVariant::fromValue(SuffixedNumber(100, "%")));

  QVERIFY(
      set_data(0, words_column, first_chord_symbol_index, QVariant("hello")));
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index),
            QVariant("hello"));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index), QVariant(""));

  QVERIFY(set_data(0, instrument_column, first_chord_symbol_index,
                    QVariant("Oboe")));
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
            QVariant("Oboe"));
  editor.undo_stack.undo();
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
            QVariant(""));

  editor.chords_model_pointer->get_node(first_note_symbol_index)
      .note_chord_pointer->setData(-1, QVariant());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  editor.chords_model_pointer->directly_set_data(root_index, QVariant());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  editor.chords_model_pointer->directly_set_data(root_index, QVariant());
}

void Tester::test_flags() {
  auto &root = editor.chords_model_pointer->root;

  // cant edit the symbol
  QCOMPARE(editor.chords_model_pointer->flags(first_chord_symbol_index),
            Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(
      editor.chords_model_pointer->flags(
          editor.chords_model_pointer->index(0, interval_column, root_index)),
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  QCOMPARE(editor.chords_model_pointer->column_flags(-1), Qt::NoItemFlags);
}

void Tester::test_get_value() {
  QCOMPARE(get_data(0, symbol_column, root_index), QVariant("♫"));
  QCOMPARE(qvariant_cast<Interval>(get_data(0, interval_column, root_index)),
            Interval(1));
  QCOMPARE(get_data(0, beats_column, root_index), QVariant(DEFAULT_BEATS));
  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, volume_percent_column, root_index)),
            SuffixedNumber(100, "%"));
  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, tempo_percent_column, root_index)),
            SuffixedNumber(100, "%"));
  QCOMPARE(get_data(0, words_column, root_index), QVariant(DEFAULT_WORDS));
  QCOMPARE(get_data(0, instrument_column, root_index), QVariant(""));

  // error on non-existent column
  QCOMPARE(
      first_chord_node_pointer->note_chord_pointer->data(-1, Qt::DisplayRole),
      QVariant());
  // empty for non-display data
  QCOMPARE(editor.chords_model_pointer->data(first_chord_symbol_index,
                                              Qt::DecorationRole),
            QVariant());

  QCOMPARE(get_data(0, symbol_column, first_chord_symbol_index),
            QVariant("♪"));
  QCOMPARE(qvariant_cast<Interval>(
                get_data(0, interval_column, first_chord_symbol_index)),
            Interval(1));
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index),
            QVariant(DEFAULT_BEATS));
  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, volume_percent_column, first_chord_symbol_index)),
            SuffixedNumber(100, "%"));
  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, tempo_percent_column, first_chord_symbol_index)),
            SuffixedNumber(100, "%"));
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index),
            QVariant(DEFAULT_WORDS));
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
            QVariant(""));

  // error on non-existent column
  QCOMPARE(
      first_note_node_pointer->note_chord_pointer->data(-1, Qt::DisplayRole),
      QVariant());
  // empty for non display data
  QCOMPARE(editor.chords_model_pointer->data(first_note_symbol_index,
                                              Qt::DecorationRole),
            QVariant());

  QTest::ignoreMessage(QtCriticalMsg, "Invalid level 0!");
  QCOMPARE(editor.chords_model_pointer->data(root_index, symbol_column),
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
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50,
    "not a field": 1
  })""""));
  // non-string starting instrument
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": 1,
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-existent starting instrument
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Not an instrument",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-double starting key
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": "",
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // below minimum starting key
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": -1,
    "starting_tempo": 200,
    "starting_volume": 50
  })""""));
  // non-double starting tempo
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": "",
    "starting_volume": 50
  })""""));
  // below minimum starting tempo
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": -1,
    "starting_volume": 50
  })""""));
  // non-double starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": ""
  })""""));
  // negative starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": -1
  })""""));
  // above maximum starting volume
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume": 101
  })""""));
  // non-array chords
  QVERIFY(!dismiss_load_text(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
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
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"beats\": 200}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"volume_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"tempo_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"words\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_chord("{\"instrument\": -1}")));
  QVERIFY(!dismiss_load_text(
      frame_json_chord("{\"instrument\": \"not an instrument\"}")));
  QVERIFY(!dismiss_load_text(R""""(
    {
      "starting_instrument": "Marimba",
      "starting_key": 220,
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
  QVERIFY(!dismiss_load_text(
      frame_json_note("{\"interval\": \"not an interval\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1/0\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1/200\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1o-20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"interval\": \"1o20\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": 1.5}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"beats\": 200}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"volume_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": \"\"}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": 0}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"tempo_percent\": 401}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"words\": -1}")));
  QVERIFY(!dismiss_load_text(frame_json_note("{\"instrument\": -1}")));
  QVERIFY(!dismiss_load_text(
      frame_json_note("{\"instrument\": \"not an instrument\"}")));

  QCOMPARE(Interval::parse_interval("1").denominator, 1);
  QCOMPARE(Interval::parse_interval("1").octave, 0);

  auto json_document = song.to_json();
}

void Tester::test_colors() {
  QCOMPARE(get_color(0, symbol_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(0, interval_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, beats_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, volume_percent_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, tempo_percent_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, words_column, root_index), DEFAULT_COLOR);
  QCOMPARE(get_color(0, instrument_column, root_index), DEFAULT_COLOR);
  QCOMPARE(first_chord_node_pointer->note_chord_pointer->data(
                -1, Qt::ForegroundRole),
            QVariant());

  QCOMPARE(get_color(1, interval_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, beats_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, volume_percent_column, root_index),
            NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, tempo_percent_column, root_index), NON_DEFAULT_COLOR);
  QCOMPARE(get_color(1, words_column, root_index), NON_DEFAULT_COLOR);

  QCOMPARE(get_color(0, symbol_column, first_chord_symbol_index),
            NON_DEFAULT_COLOR);
  QCOMPARE(get_color(0, interval_column, first_chord_symbol_index),
            DEFAULT_COLOR);
  QCOMPARE(get_color(0, beats_column, first_chord_symbol_index),
            DEFAULT_COLOR);
  QCOMPARE(get_color(0, volume_percent_column, first_chord_symbol_index),
            DEFAULT_COLOR);
  QCOMPARE(get_color(0, tempo_percent_column, first_chord_symbol_index),
            DEFAULT_COLOR);
  QCOMPARE(get_color(0, words_column, first_chord_symbol_index),
            DEFAULT_COLOR);
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
  QCOMPARE(first_note_node_pointer->note_chord_pointer->data(
                -1, Qt::ForegroundRole),
            QVariant());
}

void Tester::test_controls() {
  auto old_frequency =
      editor.starting_key_show_slider_pointer->slider_pointer->value();
  editor.starting_key_show_slider_pointer->slider_pointer->setValue(
      STARTING_KEY_1);
  QCOMPARE(song.starting_key, STARTING_KEY_1);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_key, old_frequency);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_key_show_slider_pointer->slider_pointer->value(),
           STARTING_KEY_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_key_show_slider_pointer->slider_pointer->setValue(
      STARTING_KEY_1);
  editor.starting_key_show_slider_pointer->slider_pointer->setValue(
      STARTING_KEY_2);
  QCOMPARE(song.starting_key, STARTING_KEY_2);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_key, old_frequency);

  auto old_tempo =
      editor.starting_tempo_show_slider_pointer->slider_pointer->value();
  editor.starting_tempo_show_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_1);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_tempo, old_tempo);

  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_tempo_show_slider_pointer->slider_pointer->value(),
           STARTING_TEMPO_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_tempo_show_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  editor.starting_tempo_show_slider_pointer->slider_pointer->setValue(
      STARTING_TEMPO_2);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_2);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_tempo, old_tempo);

  auto old_volume_percent =
      editor.starting_volume_show_slider_pointer->slider_pointer->value();
  editor.starting_volume_show_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  QCOMPARE(song.starting_volume, STARTING_VOLUME_1);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_volume, old_volume_percent);
  // test we actually move the slider on a redo
  editor.undo_stack.redo();
  QCOMPARE(editor.starting_volume_show_slider_pointer->slider_pointer->value(),
           STARTING_VOLUME_1);
  editor.undo_stack.undo();

  // test combining
  editor.starting_volume_show_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  editor.starting_volume_show_slider_pointer->slider_pointer->setValue(
      STARTING_VOLUME_2);
  QCOMPARE(song.starting_volume, STARTING_VOLUME_2);
  editor.undo_stack.undo();
  QCOMPARE(song.starting_volume, old_volume_percent);

  // test default instrument change
  editor.starting_instrument_selector_pointer->setCurrentText("Oboe");
  QCOMPARE(song.starting_instrument, "Oboe");
  editor.undo_stack.undo();
  QCOMPARE(song.starting_instrument, "Marimba");

  editor.starting_instrument_selector_pointer->setCurrentText("Oboe");
  editor.starting_instrument_selector_pointer->setCurrentText("Ocarina");
  QCOMPARE(song.starting_instrument, "Ocarina");
  editor.undo_stack.undo();
  QCOMPARE(song.starting_instrument, "Marimba");
}

auto Tester::dismiss_load_text(const QString &text) -> bool {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  return song.load_text(text.toUtf8());
}

void Tester::dismiss_paste(int first_index, const QString &paste_text,
                           const QModelIndex &parent_index) {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  editor.paste_text(first_index, paste_text.toUtf8(), parent_index);
}

void Tester::dismiss_save(const QString& filename)  {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  auto original_file = editor.current_file;
  editor.current_file = filename;
  editor.save();
  editor.current_file = original_file;
}

void Tester::dismiss_save_as(const QString& filename) {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  editor.save_as_file(filename);
}

void Tester::dismiss_open(const QString& filename) {
  QTimer::singleShot(MESSAGE_BOX_WAIT, this, &Tester::dismiss_messages);
  editor.open_file(filename);
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
  auto first_chord_interval_index =
      editor.chords_model_pointer->index(0, interval_column, root_index);
  auto first_chord_beats_index =
      editor.chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index = editor.chords_model_pointer->index(
      0, volume_percent_column, root_index);

  std::unique_ptr<IntervalEditor> interval_editor_pointer(
      dynamic_cast<IntervalEditor *>(
          editor.interval_delegate_pointer->createEditor(
              nullptr, QStyleOptionViewItem(), first_chord_interval_index)));

  editor.interval_delegate_pointer->updateEditorGeometry(
      interval_editor_pointer.get(), QStyleOptionViewItem(),
      first_chord_interval_index);

  QCOMPARE(interval_editor_pointer->size(),
            interval_editor_pointer->sizeHint());

  editor.interval_delegate_pointer->setEditorData(
      interval_editor_pointer.get(), first_chord_interval_index);

  QCOMPARE(interval_editor_pointer->numerator_box_pointer->value(), 1);

  interval_editor_pointer->numerator_box_pointer->setValue(2);

  editor.interval_delegate_pointer->setModelData(
      interval_editor_pointer.get(), editor.chords_model_pointer,
      first_chord_interval_index);

  QCOMPARE(qvariant_cast<Interval>(get_data(0, interval_column, root_index)),
            Interval(2));

  editor.undo_stack.undo();

  QCOMPARE(qvariant_cast<Interval>(get_data(0, interval_column, root_index)),
            Interval(1));

  std::unique_ptr<QSpinBox> spin_box_pointer(
      dynamic_cast<QSpinBox *>(editor.beats_delegate_pointer->createEditor(
          nullptr, QStyleOptionViewItem(), first_chord_beats_index)));

  editor.beats_delegate_pointer->updateEditorGeometry(
      spin_box_pointer.get(), QStyleOptionViewItem(),
      first_chord_beats_index);

  QCOMPARE(spin_box_pointer->size(), spin_box_pointer->sizeHint());

  editor.beats_delegate_pointer->setEditorData(spin_box_pointer.get(),
                                                first_chord_beats_index);

  QCOMPARE(spin_box_pointer->value(), 1);

  spin_box_pointer->setValue(2);

  editor.beats_delegate_pointer->setModelData(spin_box_pointer.get(),
                                              editor.chords_model_pointer,
                                              first_chord_beats_index);

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(2));

  editor.undo_stack.undo();

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(1));

  std::unique_ptr<ShowSlider> show_slider_pointer(dynamic_cast<ShowSlider *>(
      editor.volume_percent_delegate_pointer->createEditor(
          nullptr, QStyleOptionViewItem(),
          first_chord_volume_percent_index)));

  editor.volume_percent_delegate_pointer->updateEditorGeometry(
      show_slider_pointer.get(), QStyleOptionViewItem(),
      first_chord_volume_percent_index);

  QCOMPARE(show_slider_pointer->size(), show_slider_pointer->sizeHint());

  editor.volume_percent_delegate_pointer->setEditorData(
      show_slider_pointer.get(), first_chord_volume_percent_index);

  QCOMPARE(show_slider_pointer->slider_pointer->value(), 100);

  show_slider_pointer->set_value_no_signals(VOLUME_PERCENT_1);

  editor.volume_percent_delegate_pointer->setModelData(
      show_slider_pointer.get(), editor.chords_model_pointer,
      first_chord_volume_percent_index);

  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, volume_percent_column, root_index)),
            SuffixedNumber(101, "%"));

  editor.undo_stack.undo();

  QCOMPARE(qvariant_cast<SuffixedNumber>(
                get_data(0, volume_percent_column, root_index)),
            SuffixedNumber(100, "%"));

  std::unique_ptr<QComboBox> combo_box_delegate_pointer(
      dynamic_cast<QComboBox *>(
          editor.instrument_delegate_pointer->createEditor(
              nullptr, QStyleOptionViewItem(), first_note_instrument_index)));

  editor.instrument_delegate_pointer->updateEditorGeometry(
      combo_box_delegate_pointer.get(), QStyleOptionViewItem(),
      first_note_instrument_index);

  QCOMPARE(combo_box_delegate_pointer->size(),
            combo_box_delegate_pointer->sizeHint());

  editor.instrument_delegate_pointer->setEditorData(
      combo_box_delegate_pointer.get(), first_note_instrument_index);

  QCOMPARE(combo_box_delegate_pointer->currentText(), "");

  combo_box_delegate_pointer->setCurrentText("Oboe");

  editor.instrument_delegate_pointer->setModelData(
      combo_box_delegate_pointer.get(), editor.chords_model_pointer,
      first_note_instrument_index);

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
            QVariant("Oboe"));

  editor.undo_stack.undo();

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
            QVariant(""));
}

void Tester::test_io() {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  editor.save_as_file(temp_json_file.fileName());
  editor.save();

  QTemporaryFile temp_wav_file;
  temp_json_file.open();
  temp_json_file.close();
  editor.export_recording_file(temp_json_file.fileName());

  dismiss_save("/<>:\"/\\|?*");
  dismiss_save_as("/<>:\"/\\|?*");
  dismiss_open("/<>:\"/\\|?*");
}