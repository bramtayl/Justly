#include "test/Tester.h"

#include <qabstractitemmodel.h>   // for QModelIndex, QModelIndexList
#include <qaction.h>              // for QAction
#include <qapplication.h>         // for QApplication
#include <qglobal.h>              // for QtCriticalMsg, QFlags
#include <qitemselectionmodel.h>  // for QItemSelectionModel, opera...
#include <qlist.h>                // for QList<>::iterator, QList
#include <qmessagebox.h>          // for QMessageBox
#include <qnamespace.h>           // for operator|, DisplayRole
#include <qobject.h>              // for qobject_cast
#include <qslider.h>              // for QSlider
#include <qspinbox.h>             // for QSpinBox
#include <qstring.h>              // for QString
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtemporaryfile.h>       // for QTemporaryFile
#include <qtest.h>                // for qCompare
#include <qtestcase.h>            // for qCompare, QCOMPARE, ignore...
#include <qtestkeyboard.h>        // for keyClick
#include <qtimer.h>               // for QTimer
#include <qtreeview.h>            // for QTreeView
#include <qundostack.h>           // for QUndoStack
#include <qvariant.h>             // for QVariant, qvariant_cast
#include <qwidget.h>              // for QWidget
#include <qwindowdefs.h>          // for QWidgetList

#include <algorithm>              // for max
#include <map>                    // for operator!=
#include <memory>                 // for unique_ptr, allocator_trai...
#include <nlohmann/json.hpp>      // for basic_json, basic_json<>::...
#include <nlohmann/json_fwd.hpp>  // for json
#include <thread>                 // for sleep_for
#include <vector>                 // for vector

#include "delegates/InstrumentDelegate.h"  // for InstrumentDelegate
#include "delegates/IntervalDelegate.h"    // for IntervalDelegate
#include "delegates/ShowSliderDelegate.h"  // for ShowSliderDelegate
#include "delegates/SpinBoxDelegate.h"     // for SpinBoxDelegate
#include "editors/InstrumentEditor.h"      // for InstrumentEditor
#include "editors/IntervalEditor.h"        // for IntervalEditor
#include "editors/ShowSlider.h"            // for ShowSlider
#include "main/Editor.h"                   // for Editor
#include "main/Player.h"                   // for Player
#include "main/Song.h"                     // for Song
#include "main/TreeNode.h"                 // for TreeNode
#include "metatypes/Instrument.h"          // for Instrument
#include "metatypes/Interval.h"            // for Interval, DEFAULT_DENOMINATOR
#include "metatypes/SuffixedNumber.h"      // for SuffixedNumber
#include "models/ChordsModel.h"            // for ChordsModel
#include "notechord/NoteChord.h"           // for interval_column, beats_column

const auto STARTING_KEY_1 = 401;
const auto STARTING_KEY_2 = 402;
const auto STARTING_TEMPO_1 = 221;
const auto STARTING_TEMPO_2 = 222;
const auto STARTING_VOLUME_1 = 51;
const auto STARTING_VOLUME_2 = 52;

const auto VOLUME_PERCENT_1 = 101;

const auto NO_DATA = QVariant();

const auto BIG_ROW = 10;

auto Tester::get_wait_time() -> const std::chrono::milliseconds & {
  static const auto result = std::chrono::milliseconds(1000);
  return result;
}

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor_pointer->chords_model_pointer->headerData(
      column, Qt::Horizontal, Qt::DisplayRole);
}

auto Tester::get_data(int row, int column, QModelIndex &parent_index) const
    -> QVariant {
  return editor_pointer->chords_model_pointer->data(
      editor_pointer->chords_model_pointer->index(row, column, parent_index),
      Qt::DisplayRole);
}

auto Tester::get_color(int row, int column, QModelIndex &parent_index) const
    -> QVariant {
  return editor_pointer->chords_model_pointer->data(
      editor_pointer->chords_model_pointer->index(row, column, parent_index),
      Qt::ForegroundRole);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) const -> bool {
  return editor_pointer->chords_model_pointer->setData(
      editor_pointer->chords_model_pointer->index(row, column, parent_index),
      new_value, Qt::EditRole);
}

void Tester::initTestCase() {
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &Tester::close_one_message);
  timer_pointer->start(get_wait_time());
  if (main_file.open()) {
    main_file.write(R""""({
    "chords": [
        {
            "notes": [
                {},
                {
                    "interval": {
                      "numerator": 2,
                      "denominator": 2,
                      "octave": 1
                    },
                    "beats": 2,
                    "volume_percent": 2,
                    "tempo_percent": 2,
                    "words": "hello",
                    "instrument": "Oboe"
                }
            ]
        },
        {
            "interval": {
              "numerator": 2,
              "denominator": 2,
              "octave": 1
            },
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
  editor_pointer->open_file(main_file.fileName());

  QCOMPARE(song.root.number_of_children(), 3);
  first_chord_node_pointer =
      editor_pointer->chords_model_pointer->root_pointer->child_pointers[0]
          .get();
  QCOMPARE(first_chord_node_pointer->number_of_children(), 2);
  QCOMPARE(song.root.child_pointers[1]->number_of_children(), 1);

  first_note_node_pointer = first_chord_node_pointer->child_pointers[0].get();
  third_chord_node_pointer =
      editor_pointer->chords_model_pointer->root_pointer->child_pointers[2]
          .get();

  first_chord_symbol_index =
      editor_pointer->chords_model_pointer->index(0, symbol_column, root_index);
  second_chord_symbol_index =
      editor_pointer->chords_model_pointer->index(1, symbol_column, root_index);
  first_note_instrument_index = editor_pointer->chords_model_pointer->index(
      0, instrument_column, first_chord_symbol_index);

  first_note_symbol_index = editor_pointer->chords_model_pointer->index(
      0, symbol_column, first_chord_symbol_index);
  third_chord_symbol_index =
      editor_pointer->chords_model_pointer->index(2, symbol_column, root_index);
}

void Tester::test_column_headers_template() const {
  QFETCH(NoteChordField, field);
  QFETCH(QVariant, value);

  QCOMPARE(get_column_heading(field), value);
}

void Tester::test_column_headers_template_data() {
  QTest::addColumn<NoteChordField>("field");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("symbol header") << symbol_column << QVariant();
  QTest::newRow("interval header") << interval_column << QVariant("Interval");
  QTest::newRow("beats header") << beats_column << QVariant("Beats");
  QTest::newRow("volume header") << volume_percent_column << QVariant("Volume");
  QTest::newRow("tempo header") << tempo_percent_column << QVariant("Tempo");
  QTest::newRow("words header") << words_column << QVariant("Words");
  QTest::newRow("instruments header")
      << instrument_column << QVariant("Instrument");
}

void Tester::test_column_headers() const {
  // error for non-existent column
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(editor_pointer->chords_model_pointer->headerData(
               interval_column, Qt::Vertical, Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(editor_pointer->chords_model_pointer->headerData(
               interval_column, Qt::Horizontal, Qt::DecorationRole),
           QVariant());
}

void Tester::test_view() const {
  editor_pointer->show();
  editor_pointer->view_controls_checkbox_pointer->setChecked(false);
  QVERIFY(!(editor_pointer->controls_pointer->isVisible()));
  editor_pointer->view_controls_checkbox_pointer->setChecked(true);
  QVERIFY(editor_pointer->controls_pointer->isVisible());
  editor_pointer->close();
}

void Tester::test_insert_delete() {
  select_index(first_chord_symbol_index);
  editor_pointer->insert_before();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      4);
  editor_pointer->undo_stack.undo();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      3);
  clear_selection();
  editor_pointer->insert_before();

  select_index(first_chord_symbol_index);
  editor_pointer->insert_after();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      4);
  editor_pointer->undo_stack.undo();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      3);
  clear_selection();
  QCOMPARE(editor_pointer->chords_view_pointer->selectionModel()
               ->selectedRows()
               .size(),
           0);
  editor_pointer->insert_after();

  select_index(third_chord_symbol_index);
  editor_pointer->insert_into();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 0);
  clear_selection();
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor_pointer->insert_into();

  select_index(first_chord_symbol_index);
  editor_pointer->remove_selected();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      2);
  editor_pointer->undo_stack.undo();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      3);
  clear_selection();
  editor_pointer->remove_selected();

  select_index(first_note_symbol_index);
  editor_pointer->insert_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor_pointer->undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor_pointer->insert_before();

  select_index(first_note_symbol_index);
  editor_pointer->insert_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor_pointer->undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor_pointer->insert_after();

  select_index(first_note_symbol_index);
  editor_pointer->remove_selected();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor_pointer->remove_selected();
}

void Tester::test_copy_paste() {
  select_index(first_chord_symbol_index);
  editor_pointer->copy_selected();
  clear_selection();
  editor_pointer->copy_selected();

  // paste after first chord
  select_index(first_chord_symbol_index);
  editor_pointer->paste_before();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      4);
  editor_pointer->undo_stack.undo();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      3);
  clear_selection();
  editor_pointer->paste_before();

  select_index(first_chord_symbol_index);
  editor_pointer->paste_after();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      4);
  editor_pointer->undo_stack.undo();
  QCOMPARE(
      editor_pointer->chords_model_pointer->root_pointer->child_pointers.size(),
      3);
  clear_selection();
  editor_pointer->paste_after();

  select_index(first_note_symbol_index);
  editor_pointer->copy_selected();
  clear_selection();
  editor_pointer->copy_selected();

  select_index(first_note_symbol_index);
  editor_pointer->paste_before();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor_pointer->undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor_pointer->paste_before();

  select_index(first_note_symbol_index);
  editor_pointer->paste_after();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 3);
  editor_pointer->undo_stack.undo();
  QCOMPARE(first_chord_node_pointer->child_pointers.size(), 2);
  clear_selection();
  editor_pointer->paste_after();

  select_index(third_chord_symbol_index);
  editor_pointer->paste_into();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(third_chord_node_pointer->child_pointers.size(), 0);
  clear_selection();

  editor_pointer->paste_text(0, "[", root_index);

  editor_pointer->paste_text(0, "{}", root_index);

  editor_pointer->paste_text(0, "[", first_chord_symbol_index);

  editor_pointer->paste_text(0, "{}", first_chord_symbol_index);

  std::vector<std::unique_ptr<TreeNode>> insertion;
  insertion.push_back(std::make_unique<TreeNode>(first_chord_node_pointer));
  QTest::ignoreMessage(
      QtCriticalMsg,
      "Parent with level 0 cannot contain children with level 2!");
  song.root.insert_children(0, insertion);
}

void Tester::test_play_template() const {
  if (editor_pointer->player_pointer->performer_pointer != nullptr) {
    QFETCH(QModelIndex, first_index);
    QFETCH(QModelIndex, last_index);

    select_indices(first_index, last_index);
    // use the second chord to test key changing
    editor_pointer->play_selected();
    // first cut off early
    editor_pointer->play_selected();
    // now play the whole thing
    std::this_thread::sleep_for(get_wait_time());
    clear_selection();
  }
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << first_chord_symbol_index << second_chord_symbol_index;
  QTest::newRow("second chord")
      << second_chord_symbol_index << second_chord_symbol_index;

  auto first_chord_second_note_index =
      editor_pointer->chords_model_pointer->index(1, symbol_column,
                                                  first_chord_symbol_index);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index =
      editor_pointer->chords_model_pointer->index(0, symbol_column,
                                                  second_chord_symbol_index);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_play() const {
  if (editor_pointer->player_pointer->performer_pointer != nullptr) {
    editor_pointer->play_selected();
    editor_pointer->stop_playing();
  }
}

void Tester::select_indices(const QModelIndex first_index,
                            const QModelIndex last_index) const {
  auto chord_selection = QItemSelection(first_index, last_index);
  editor_pointer->chords_view_pointer->selectionModel()->blockSignals(true);
  editor_pointer->chords_view_pointer->selectionModel()->select(
      chord_selection, QItemSelectionModel::Current |
                           QItemSelectionModel::Select |
                           QItemSelectionModel::Rows);
  editor_pointer->chords_view_pointer->selectionModel()->blockSignals(false);
}

void Tester::select_index(const QModelIndex index) const {
  select_indices(index, index);
}

void Tester::clear_selection() const {
  editor_pointer->chords_view_pointer->selectionModel()->select(
      QItemSelection(),
      QItemSelectionModel::Current | QItemSelectionModel::Clear);
}

void Tester::test_tree() {
  auto &root_pointer = editor_pointer->chords_model_pointer->root_pointer;

  const TreeNode untethered(root_pointer);
  QTest::ignoreMessage(QtCriticalMsg, "Not a child!");
  QCOMPARE(untethered.get_row(), -1);
  // test song
  QCOMPARE(editor_pointer->chords_model_pointer->rowCount(root_index), 3);
  QCOMPARE(editor_pointer->chords_model_pointer->columnCount(QModelIndex()),
           NOTE_CHORD_COLUMNS);
  QCOMPARE(editor_pointer->chords_model_pointer->root_pointer->get_level(),
           root_level);

  // test first chord
  QCOMPARE(first_chord_node_pointer->get_level(), chord_level);
  QCOMPARE(
      editor_pointer->chords_model_pointer->parent(first_chord_symbol_index),
      root_index);
  // only nest the symbol column
  QCOMPARE(editor_pointer->chords_model_pointer->rowCount(
               editor_pointer->chords_model_pointer->index(0, interval_column,
                                                           root_index)),
           0);

  // test first note
  QCOMPARE(editor_pointer->chords_model_pointer->parent(first_note_symbol_index)
               .row(),
           0);
  QCOMPARE(first_note_node_pointer->get_level(), note_level);

  QTest::ignoreMessage(QtCriticalMsg, "Notes can't have children!");
  QCOMPARE(
      first_note_node_pointer->get_note_chord().new_child_pointer().get(),
      nullptr);
}

void Tester::test_set_value_template() {
  QFETCH(int, row);
  QFETCH(NoteChordField, field);
  QFETCH(QModelIndex, parent_index);
  QFETCH(QVariant, old_value);
  QFETCH(QVariant, new_value);

  auto index =
      editor_pointer->chords_model_pointer->index(row, field, parent_index);

  QVERIFY(editor_pointer->chords_model_pointer->setData(index, new_value,
                                                        Qt::EditRole));
  QCOMPARE(editor_pointer->chords_model_pointer->data(index, Qt::DisplayRole),
           new_value);

  editor_pointer->undo_stack.undo();
  editor_pointer->undo_stack.redo();
  editor_pointer->undo_stack.undo();

  QCOMPARE(editor_pointer->chords_model_pointer->data(index, Qt::DisplayRole),
           old_value);
}

void Tester::test_set_value() {
  QVERIFY(set_data(0, symbol_column, root_index, QVariant()));

  editor_pointer->chords_model_pointer->get_node(first_chord_symbol_index)
      .get_note_chord().setData(-1, QVariant());
  // setData only works for the edit role
  QVERIFY(!(editor_pointer->chords_model_pointer->setData(
      first_chord_symbol_index, QVariant(), Qt::DecorationRole)));

  editor_pointer->chords_model_pointer->get_node(first_note_symbol_index)
      .get_note_chord().setData(-1, QVariant());

}

void Tester::test_set_value_template_data() {
  QTest::addColumn<int>("row");
  QTest::addColumn<NoteChordField>("field");
  QTest::addColumn<QModelIndex>("parent_index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("first_chord_interval")
      << 0 << interval_column << root_index << QVariant::fromValue(Interval())
      << QVariant::fromValue(Interval(2));
  QTest::newRow("first_chord_beats")
      << 0 << beats_column << root_index << QVariant::fromValue(1)
      << QVariant::fromValue(2);
  QTest::newRow("first_chord_volume")
      << 0 << volume_percent_column << root_index
      << QVariant::fromValue(SuffixedNumber(PERCENT, "%"))
      << QVariant::fromValue(SuffixedNumber(2, "%"));
  QTest::newRow("first_chord_tempo")
      << 0 << tempo_percent_column << root_index
      << QVariant::fromValue(SuffixedNumber(PERCENT, "%"))
      << QVariant::fromValue(SuffixedNumber(2, "%"));
  QTest::newRow("first_chord_words")
      << 0 << words_column << root_index << QVariant("") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << 0 << instrument_column << root_index
      << QVariant::fromValue(Instrument::get_instrument_by_name(""))
      << QVariant::fromValue(Instrument::get_instrument_by_name("Oboe"));

  QTest::newRow("first_note_interval")
      << 0 << interval_column << first_chord_symbol_index
      << QVariant::fromValue(Interval()) << QVariant::fromValue(Interval(2));
  QTest::newRow("first_note_beats")
      << 0 << beats_column << first_chord_symbol_index << QVariant::fromValue(1)
      << QVariant::fromValue(2);
  QTest::newRow("first_note_volume")
      << 0 << volume_percent_column << first_chord_symbol_index
      << QVariant::fromValue(SuffixedNumber(PERCENT, "%"))
      << QVariant::fromValue(SuffixedNumber(2, "%"));
  QTest::newRow("first_note_tempo")
      << 0 << tempo_percent_column << first_chord_symbol_index
      << QVariant::fromValue(SuffixedNumber(PERCENT, "%"))
      << QVariant::fromValue(SuffixedNumber(2, "%"));
  QTest::newRow("first_note_words")
      << 0 << words_column << first_chord_symbol_index << QVariant("")
      << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << 0 << instrument_column << first_chord_symbol_index
      << QVariant::fromValue(Instrument::get_instrument_by_name(""))
      << QVariant::fromValue(Instrument::get_instrument_by_name("Oboe"));
}

void Tester::test_flags() const {
  // cant edit the symbol
  QCOMPARE(
      editor_pointer->chords_model_pointer->flags(first_chord_symbol_index),
      Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(editor_pointer->chords_model_pointer->flags(
               editor_pointer->chords_model_pointer->index(0, interval_column,
                                                           root_index)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  QCOMPARE(editor_pointer->chords_model_pointer->column_flags(-1),
           Qt::NoItemFlags);
}

void Tester::test_get_value() {
  QCOMPARE(get_data(0, symbol_column, root_index), QVariant("♫"));

  // error on non-existent column
  QCOMPARE(
      first_chord_node_pointer->get_const_note_chord().data(-1, Qt::DisplayRole),
      QVariant());
  // empty for non-display data
  QCOMPARE(editor_pointer->chords_model_pointer->data(first_chord_symbol_index,
                                                      Qt::DecorationRole),
           QVariant());

  QCOMPARE(get_data(0, symbol_column, first_chord_symbol_index), QVariant("♪"));

  // error on non-existent column
  QCOMPARE(
      first_note_node_pointer->get_const_note_chord().data(-1, Qt::DisplayRole),
      QVariant());
  // empty for non display data
  QCOMPARE(editor_pointer->chords_model_pointer->data(first_note_symbol_index,
                                                      Qt::DecorationRole),
           QVariant());

  auto test_interval = Interval();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.get_text(), "1/2");
  test_interval.denominator = DEFAULT_DENOMINATOR;
  test_interval.octave = 1;
  QCOMPARE(test_interval.get_text(), "1o1");
}

void Tester::test_json() {
  QVERIFY(!song.load_text("{"));
  QVERIFY(!song.load_text("{}"));
  auto json_document = song.to_json();
}

void Tester::test_colors_template() {
  QFETCH(int, row);
  QFETCH(NoteChordField, field);
  QFETCH(QModelIndex, parent_index);
  QFETCH(bool, non_default);

  if (non_default) {
    QCOMPARE(get_color(row, field, root_index), NON_DEFAULT_COLOR);
  } else {
    QCOMPARE(get_color(row, field, root_index), DEFAULT_COLOR);
  }
}

void Tester::test_colors_template_data() {
  QTest::addColumn<int>("row");
  QTest::addColumn<NoteChordField>("field");
  QTest::addColumn<QModelIndex>("parent_index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color")
      << 0 << symbol_column << root_index << true;
  QTest::newRow("first_chord_interval_color")
      << 0 << interval_column << root_index << false;
  QTest::newRow("first_chord_beats_color")
      << 0 << beats_column << root_index << false;
  QTest::newRow("first_chord_volume_color")
      << 0 << volume_percent_column << root_index << false;
  QTest::newRow("first_chord_tempo_color")
      << 0 << tempo_percent_column << root_index << false;
  QTest::newRow("first_chord_words_color")
      << 0 << words_column << root_index << false;
  QTest::newRow("first_chord_instrument_color")
      << 0 << instrument_column << root_index << false;

  QTest::newRow("second_chord_interval_color")
      << 1 << interval_column << root_index << true;
  QTest::newRow("second_chord_beats_color")
      << 1 << beats_column << root_index << true;
  QTest::newRow("second_chord_volume_color")
      << 1 << volume_percent_column << root_index << true;
  QTest::newRow("second_chord_tempo_color")
      << 1 << tempo_percent_column << root_index << true;
  QTest::newRow("second_chord_words_color")
      << 1 << words_column << root_index << true;

  QTest::newRow("first_note_symbol_color")
      << 0 << symbol_column << root_index << true;
  QTest::newRow("first_note_interval_color")
      << 0 << interval_column << first_chord_symbol_index << false;
  QTest::newRow("first_note_beats_color")
      << 0 << beats_column << first_chord_symbol_index << false;
  QTest::newRow("first_note_volume_color")
      << 0 << volume_percent_column << first_chord_symbol_index << false;
  QTest::newRow("first_note_tempo_color")
      << 0 << tempo_percent_column << first_chord_symbol_index << false;
  QTest::newRow("first_note_words_color")
      << 0 << words_column << first_chord_symbol_index << false;
  QTest::newRow("first_note_instrument_color")
      << 0 << instrument_column << first_chord_symbol_index << false;

  QTest::newRow("second_note_interval_color")
      << 1 << interval_column << first_chord_symbol_index << true;
  QTest::newRow("second_note_beats_color")
      << 1 << beats_column << first_chord_symbol_index << true;
  QTest::newRow("second_note_volume_color")
      << 1 << volume_percent_column << first_chord_symbol_index << true;
  QTest::newRow("second_note_tempo_color")
      << 1 << tempo_percent_column << first_chord_symbol_index << true;
  QTest::newRow("second_note_words_color")
      << 1 << words_column << first_chord_symbol_index << true;
  QTest::newRow("second_note_instrument_color")
      << 1 << instrument_column << first_chord_symbol_index << true;
}

void Tester::test_colors() {
  QCOMPARE(first_chord_node_pointer->get_const_note_chord().data(
               -1, Qt::ForegroundRole),
           QVariant());

  // error on non-existent column
  QCOMPARE(
      first_note_node_pointer->get_const_note_chord().data(-1, Qt::ForegroundRole),
      QVariant());
}

void Tester::test_controls() const {
  auto old_frequency =
      editor_pointer->starting_key_editor_pointer->slider_pointer->value();
  editor_pointer->starting_key_editor_pointer->slider_pointer->setValue(
      STARTING_KEY_1);
  QCOMPARE(song.starting_key, STARTING_KEY_1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_key, old_frequency);
  // test we actually move the slider on a redo
  editor_pointer->undo_stack.redo();
  QCOMPARE(editor_pointer->starting_key_editor_pointer->slider_pointer->value(),
           STARTING_KEY_1);
  editor_pointer->undo_stack.undo();

  // test combining
  editor_pointer->starting_key_editor_pointer->slider_pointer->setValue(
      STARTING_KEY_1);
  editor_pointer->starting_key_editor_pointer->slider_pointer->setValue(
      STARTING_KEY_2);
  QCOMPARE(song.starting_key, STARTING_KEY_2);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_key, old_frequency);

  auto old_tempo =
      editor_pointer->starting_tempo_editor_pointer->slider_pointer->value();
  editor_pointer->starting_tempo_editor_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_tempo, old_tempo);

  // test we actually move the slider on a redo
  editor_pointer->undo_stack.redo();
  QCOMPARE(
      editor_pointer->starting_tempo_editor_pointer->slider_pointer->value(),
      STARTING_TEMPO_1);
  editor_pointer->undo_stack.undo();

  // test combining
  editor_pointer->starting_tempo_editor_pointer->slider_pointer->setValue(
      STARTING_TEMPO_1);
  editor_pointer->starting_tempo_editor_pointer->slider_pointer->setValue(
      STARTING_TEMPO_2);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_2);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_tempo, old_tempo);

  auto old_volume_percent =
      editor_pointer->starting_volume_editor_pointer->slider_pointer->value();
  editor_pointer->starting_volume_editor_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  QCOMPARE(song.starting_volume, STARTING_VOLUME_1);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_volume, old_volume_percent);
  // test we actually move the slider on a redo
  editor_pointer->undo_stack.redo();
  QCOMPARE(
      editor_pointer->starting_volume_editor_pointer->slider_pointer->value(),
      STARTING_VOLUME_1);
  editor_pointer->undo_stack.undo();

  // test combining
  editor_pointer->starting_volume_editor_pointer->slider_pointer->setValue(
      STARTING_VOLUME_1);
  editor_pointer->starting_volume_editor_pointer->slider_pointer->setValue(
      STARTING_VOLUME_2);
  QCOMPARE(song.starting_volume, STARTING_VOLUME_2);
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_volume, old_volume_percent);

  // test default instrument change
  editor_pointer->starting_instrument_editor_pointer->set_instrument(
      Instrument::get_instrument_by_name("Oboe"));
  QCOMPARE(song.starting_instrument.instrument_name, "Oboe");
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_instrument.instrument_name, "Marimba");

  editor_pointer->starting_instrument_editor_pointer->set_instrument(
      Instrument::get_instrument_by_name("Oboe"));
  editor_pointer->starting_instrument_editor_pointer->set_instrument(
      Instrument::get_instrument_by_name("Ocarina"));
  QCOMPARE(song.starting_instrument.instrument_name, "Ocarina");
  editor_pointer->undo_stack.undo();
  QCOMPARE(song.starting_instrument.instrument_name, "Marimba");
}

void Tester::save_to(const QString &filename) const {
  auto original_file = editor_pointer->current_file;
  editor_pointer->current_file = filename;
  editor_pointer->save();
  editor_pointer->current_file = original_file;
}

void Tester::close_one_message() {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *box_pointer = qobject_cast<QMessageBox *>(widget_pointer);
    if (box_pointer != nullptr) {
      QTest::keyEvent(QTest::Press, qobject_cast<QMessageBox *>(box_pointer),
                      Qt::Key_Enter);
      return;
    }
  }
}

void Tester::test_select() const {
  auto item_selection =
      QItemSelection(first_chord_symbol_index, second_chord_symbol_index);
  editor_pointer->chords_view_pointer->selectionModel()->select(
      item_selection, QItemSelectionModel::Select);
  editor_pointer->chords_view_pointer->selectionModel()->select(
      item_selection, QItemSelectionModel::Deselect);
}

void Tester::test_io() const {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  editor_pointer->save_as_file(temp_json_file.fileName());
  editor_pointer->save();

  const QTemporaryFile temp_wav_file;
  temp_json_file.open();
  temp_json_file.close();
  // editor_pointer->export_recording_file(temp_json_file.fileName());

  save_to("/<>:\"/\\|?*");
  editor_pointer->save_as_file("/<>:\"/\\|?*");
  editor_pointer->open_file("/<>:\"/\\|?*");
}

void Tester::test_beats_delegate() {
  auto first_chord_interval_index = editor_pointer->chords_model_pointer->index(
      0, interval_column, root_index);
  auto first_chord_beats_index =
      editor_pointer->chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index =
      editor_pointer->chords_model_pointer->index(0, volume_percent_column,
                                                  root_index);

  QWidget parent_widget;

  QSpinBox *spin_box_pointer = dynamic_cast<QSpinBox *>(
      editor_pointer->beats_delegate_pointer->createEditor(
          &parent_widget, QStyleOptionViewItem(), first_chord_beats_index));

  editor_pointer->beats_delegate_pointer->updateEditorGeometry(
      spin_box_pointer, QStyleOptionViewItem(), first_chord_beats_index);

  QCOMPARE(spin_box_pointer->size(), spin_box_pointer->sizeHint());

  editor_pointer->beats_delegate_pointer->setEditorData(
      spin_box_pointer, first_chord_beats_index);

  QCOMPARE(spin_box_pointer->value(), 1);

  spin_box_pointer->setValue(2);

  editor_pointer->beats_delegate_pointer->setModelData(
      spin_box_pointer, editor_pointer->chords_model_pointer,
      first_chord_beats_index);

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(2));

  editor_pointer->undo_stack.undo();

  QCOMPARE(get_data(0, beats_column, root_index), QVariant(1));
}

void Tester::test_slider_delegate() {
  auto first_chord_interval_index = editor_pointer->chords_model_pointer->index(
      0, interval_column, root_index);
  auto first_chord_beats_index =
      editor_pointer->chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index =
      editor_pointer->chords_model_pointer->index(0, volume_percent_column,
                                                  root_index);

  auto *view_port_pointer = editor_pointer->chords_view_pointer->viewport();

  ShowSlider *show_slider_pointer = dynamic_cast<ShowSlider *>(
      editor_pointer->volume_percent_delegate_pointer->createEditor(
          view_port_pointer, QStyleOptionViewItem(),
          first_chord_volume_percent_index));

  editor_pointer->volume_percent_delegate_pointer->updateEditorGeometry(
      show_slider_pointer, QStyleOptionViewItem(),
      first_chord_volume_percent_index);

  QCOMPARE(show_slider_pointer->size(), show_slider_pointer->sizeHint());

  editor_pointer->volume_percent_delegate_pointer->setEditorData(
      show_slider_pointer, first_chord_volume_percent_index);

  QCOMPARE(show_slider_pointer->slider_pointer->value(), 100);

  show_slider_pointer->set_value_no_signals(VOLUME_PERCENT_1);

  editor_pointer->volume_percent_delegate_pointer->setModelData(
      show_slider_pointer, editor_pointer->chords_model_pointer,
      first_chord_volume_percent_index);

  QCOMPARE(qvariant_cast<SuffixedNumber>(
               get_data(0, volume_percent_column, root_index)),
           SuffixedNumber(101, "%"));

  editor_pointer->undo_stack.undo();

  QCOMPARE(qvariant_cast<SuffixedNumber>(
               get_data(0, volume_percent_column, root_index)),
           SuffixedNumber(100, "%"));
}

void Tester::test_instrument_delegate() {
  auto first_chord_interval_index = editor_pointer->chords_model_pointer->index(
      0, interval_column, root_index);
  auto first_chord_beats_index =
      editor_pointer->chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index =
      editor_pointer->chords_model_pointer->index(0, volume_percent_column,
                                                  root_index);

  auto *view_port_pointer = editor_pointer->chords_view_pointer->viewport();

  InstrumentEditor *instrument_editor_pointer =
      dynamic_cast<InstrumentEditor *>(
          editor_pointer->instrument_delegate_pointer->createEditor(
              view_port_pointer, QStyleOptionViewItem(),
              first_note_instrument_index));

  editor_pointer->instrument_delegate_pointer->updateEditorGeometry(
      instrument_editor_pointer, QStyleOptionViewItem(),
      first_note_instrument_index);

  QCOMPARE(instrument_editor_pointer->size(),
           instrument_editor_pointer->sizeHint());

  editor_pointer->instrument_delegate_pointer->setEditorData(
      instrument_editor_pointer, first_note_instrument_index);

  QCOMPARE(instrument_editor_pointer->get_instrument().instrument_name, "");

  instrument_editor_pointer->set_instrument(
      Instrument::get_instrument_by_name("Oboe"));

  editor_pointer->instrument_delegate_pointer->setModelData(
      instrument_editor_pointer, editor_pointer->chords_model_pointer,
      first_note_instrument_index);

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
           QVariant::fromValue(Instrument::get_instrument_by_name("Oboe")));

  editor_pointer->undo_stack.undo();

  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index),
           QVariant::fromValue(Instrument::get_instrument_by_name("")));
}

void Tester::test_interval_delegate() {
  auto first_chord_interval_index = editor_pointer->chords_model_pointer->index(
      0, interval_column, root_index);
  auto first_chord_beats_index =
      editor_pointer->chords_model_pointer->index(0, beats_column, root_index);
  auto first_chord_volume_percent_index =
      editor_pointer->chords_model_pointer->index(0, volume_percent_column,
                                                  root_index);

  auto *view_port_pointer = editor_pointer->chords_view_pointer->viewport();

  IntervalEditor *interval_editor_pointer = dynamic_cast<IntervalEditor *>(
      editor_pointer->interval_delegate_pointer->createEditor(
          view_port_pointer, QStyleOptionViewItem(),
          first_chord_interval_index));

  editor_pointer->interval_delegate_pointer->updateEditorGeometry(
      interval_editor_pointer, QStyleOptionViewItem(),
      first_chord_interval_index);

  QCOMPARE(interval_editor_pointer->size(),
           interval_editor_pointer->sizeHint());

  editor_pointer->interval_delegate_pointer->setEditorData(
      interval_editor_pointer, first_chord_interval_index);

  QCOMPARE(interval_editor_pointer->numerator_box_pointer->value(), 1);

  interval_editor_pointer->numerator_box_pointer->setValue(2);

  editor_pointer->interval_delegate_pointer->setModelData(
      interval_editor_pointer, editor_pointer->chords_model_pointer,
      first_chord_interval_index);

  QCOMPARE(qvariant_cast<Interval>(get_data(0, interval_column, root_index)),
           Interval(2));

  editor_pointer->undo_stack.undo();

  QCOMPARE(qvariant_cast<Interval>(get_data(0, interval_column, root_index)),
           Interval(1));
}
