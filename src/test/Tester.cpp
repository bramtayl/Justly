#include "test/Tester.h"

#include <qabstractitemmodel.h>   // for QModelIndex, QModelIndexList
#include <qapplication.h>         // for QApplication
#include <qglobal.h>              // for QFlags, QtCriticalMsg
#include <qitemselectionmodel.h>  // for QItemSelectionModel, operator|
#include <qlist.h>                // for QList<>::iterator, QList
#include <qmessagebox.h>          // for QMessageBox
#include <qnamespace.h>           // for DisplayRole, operator|, Decoration...
#include <qobject.h>              // for qobject_cast
#include <qspinbox.h>
#include <qstring.h>              // for QString
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtemporaryfile.h>       // for QTemporaryFile
#include <qtest.h>                // for qCompare
#include <qtestcase.h>            // for newRow, qCompare, addColumn, QCOMPARE
#include <qtestdata.h>            // for operator<<, QTestData
#include <qtestkeyboard.h>        // for keyEvent, Press
#include <qtimer.h>               // for QTimer
#include <qvariant.h>             // for QVariant
#include <qwidget.h>              // for QWidget
#include <qwindowdefs.h>          // for QWidgetList

#include <chrono>       // for milliseconds
#include <memory>       // for unique_ptr, allocator, make_unique
#include <thread>       // for sleep_for
#include <type_traits>  // for enable_if_t
#include <vector>       // for vector

#include "editors/InstrumentEditor.h"
#include "editors/IntervalEditor.h"
#include "main/Editor.h"           // for Editor
#include "main/MyDelegate.h"       // for MyDelegate
#include "main/Player.h"           // for PERCENT
#include "main/Song.h"             // for Song
#include "main/TreeNode.h"         // for TreeNode
#include "metatypes/Instrument.h"  // for Instrument
#include "metatypes/Interval.h"    // for Interval, DEFAULT_DENOMINATOR
#include "models/ChordsModel.h"    // for ChordsModel
#include "notechord/NoteChord.h"   // for NoteChordField, interval_column
#include "utilities/utilities.h"   // for StartingFieldId, starting_instrume...

const auto PERCENT = 100;

const auto ORIGINAL_KEY = 220.0;
const auto STARTING_KEY_1 = 401.0;
const auto STARTING_KEY_2 = 402.0;
const auto ORIGINAL_TEMPO = 200.0;
const auto STARTING_TEMPO_1 = 221.0;
const auto STARTING_TEMPO_2 = 222.0;
const auto ORIGINAL_VOLUME = 50.0;
const auto STARTING_VOLUME_1 = 51.0;
const auto STARTING_VOLUME_2 = 52.0;

const auto VOLUME_PERCENT_1 = 101;
const auto TEMPO_PERCENT_1 = 101;

const auto NO_DATA = QVariant();

const auto WAIT_TIME = 1000;

const auto ROOT_INDEX = QModelIndex();

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor_pointer->get_chords_model().headerData(column, Qt::Horizontal,
                                                       Qt::DisplayRole);
}

void Tester::initTestCase() {
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &Tester::close_one_message);
  timer_pointer->start(WAIT_TIME);
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

  const auto &root = editor_pointer->get_song().root;
  const auto &chord_pointers = root.get_child_pointers();

  QCOMPARE(root.number_of_children(), 3);
  QCOMPARE(chord_pointers[0]->number_of_children(), 2);
  QCOMPARE(chord_pointers[1]->number_of_children(), 1);
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
  auto &chords_model = editor_pointer->get_chords_model();
  // error for non-existent column
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(
      chords_model.headerData(interval_column, Qt::Vertical, Qt::DisplayRole),
      QVariant());
  // headers only for display role
  QCOMPARE(chords_model.headerData(interval_column, Qt::Horizontal,
                                   Qt::DecorationRole),
           QVariant());
}

void Tester::test_view() const {
  editor_pointer->show();
  editor_pointer->set_controls_visible(false);
  QVERIFY(!(editor_pointer->are_controls_visible()));
  editor_pointer->set_controls_visible(true);
  QVERIFY(editor_pointer->are_controls_visible());
  editor_pointer->close();
}

void Tester::test_insert_delete() {
  const auto &first_chord_node =
      *editor_pointer->get_song().root.get_child_pointers()[0];
  const auto &third_chord_node =
      *editor_pointer->get_song().root.get_child_pointers()[2];
  const auto first_chord_symbol_index =
      editor_pointer->get_chords_model().index(0, symbol_column, ROOT_INDEX);
  const auto third_chord_symbol_index =
      editor_pointer->get_chords_model().index(2, symbol_column, ROOT_INDEX);
  const auto first_note_symbol_index = editor_pointer->get_chords_model().index(
      0, symbol_column, first_chord_symbol_index);

  select_index(first_chord_symbol_index);
  editor_pointer->insert_before();

  const auto &root = editor_pointer->get_song().root;
  QCOMPARE(root.number_of_children(), 4);
  editor_pointer->undo();
  QCOMPARE(root.number_of_children(), 3);
  clear_selection();
  editor_pointer->insert_before();

  select_index(first_chord_symbol_index);
  editor_pointer->insert_after();
  QCOMPARE(root.number_of_children(), 4);
  editor_pointer->undo();
  QCOMPARE(root.number_of_children(), 3);
  clear_selection();
  QCOMPARE(editor_pointer->get_selection_model().selectedRows().size(), 0);
  editor_pointer->insert_after();

  select_index(third_chord_symbol_index);
  editor_pointer->insert_into();
  QCOMPARE(third_chord_node.number_of_children(), 1);
  editor_pointer->undo();
  QCOMPARE(third_chord_node.number_of_children(), 0);
  clear_selection();
  // QTest::ignoreMessage(QtCriticalMsg, "Nothing selected!");
  // editor_pointer->insert_into();

  select_index(first_chord_symbol_index);
  editor_pointer->remove_selected();
  QCOMPARE(root.number_of_children(), 2);
  editor_pointer->undo();
  QCOMPARE(root.number_of_children(), 3);
  clear_selection();
  editor_pointer->remove_selected();

  select_index(first_note_symbol_index);
  editor_pointer->insert_before();
  QCOMPARE(first_chord_node.number_of_children(), 3);
  editor_pointer->undo();
  QCOMPARE(first_chord_node.number_of_children(), 2);
  clear_selection();
  editor_pointer->insert_before();

  select_index(first_note_symbol_index);
  editor_pointer->insert_after();
  QCOMPARE(first_chord_node.number_of_children(), 3);
  editor_pointer->undo();
  QCOMPARE(first_chord_node.number_of_children(), 2);
  clear_selection();
  editor_pointer->insert_after();

  select_index(first_note_symbol_index);
  editor_pointer->remove_selected();
  QCOMPARE(first_chord_node.number_of_children(), 1);
  editor_pointer->undo();
  QCOMPARE(first_chord_node.number_of_children(), 2);
  clear_selection();
  editor_pointer->remove_selected();
}

void Tester::test_copy_paste() {
  const auto &chord_pointers =
      editor_pointer->get_song().root.get_child_pointers();
  auto &first_chord_node = *chord_pointers[0];
  const auto &third_chord_node = *chord_pointers[2];
  const auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto third_chord_symbol_index =
      chords_model.index(2, symbol_column, ROOT_INDEX);
  const auto first_note_symbol_index =
      chords_model.index(0, symbol_column, first_chord_symbol_index);

  select_index(first_chord_symbol_index);
  editor_pointer->copy_selected();
  clear_selection();
  editor_pointer->copy_selected();

  const auto &root = editor_pointer->get_song().root;

  // paste after first chord
  select_index(first_chord_symbol_index);
  editor_pointer->paste_before();
  QCOMPARE(root.number_of_children(), 4);
  editor_pointer->undo();
  QCOMPARE(root.number_of_children(), 3);
  clear_selection();
  editor_pointer->paste_before();

  select_index(first_chord_symbol_index);
  editor_pointer->paste_after();
  QCOMPARE(root.number_of_children(), 4);
  editor_pointer->undo();
  QCOMPARE(root.number_of_children(), 3);
  clear_selection();
  editor_pointer->paste_after();

  select_index(first_note_symbol_index);
  editor_pointer->copy_selected();
  clear_selection();
  editor_pointer->copy_selected();

  select_index(first_note_symbol_index);
  editor_pointer->paste_before();
  QCOMPARE(first_chord_node.number_of_children(), 3);
  editor_pointer->undo();
  QCOMPARE(first_chord_node.number_of_children(), 2);
  clear_selection();
  editor_pointer->paste_before();

  select_index(first_note_symbol_index);
  editor_pointer->paste_after();
  QCOMPARE(first_chord_node.number_of_children(), 3);
  editor_pointer->undo();
  QCOMPARE(first_chord_node.number_of_children(), 2);
  clear_selection();
  editor_pointer->paste_after();

  select_index(third_chord_symbol_index);
  editor_pointer->paste_into();
  QCOMPARE(third_chord_node.number_of_children(), 1);
  editor_pointer->undo();
  QCOMPARE(third_chord_node.number_of_children(), 0);
  clear_selection();

  editor_pointer->paste_text(0, "[", ROOT_INDEX);

  editor_pointer->paste_text(0, "{}", ROOT_INDEX);

  editor_pointer->paste_text(0, "[", first_chord_symbol_index);

  editor_pointer->paste_text(0, "{}", first_chord_symbol_index);

  std::vector<std::unique_ptr<TreeNode>> insertion;
  insertion.push_back(std::make_unique<TreeNode>(&first_chord_node));
}

void Tester::test_play_template() const {
  if (editor_pointer->has_real_time()) {
    QFETCH(QModelIndex, first_index);
    QFETCH(QModelIndex, last_index);

    select_indices(first_index, last_index);
    // use the second chord to test key changing
    editor_pointer->play_selected();
    // first cut off early
    editor_pointer->play_selected();
    // now play the whole thing
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
    clear_selection();
  }
}

void Tester::test_play_template_data() const {
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto second_chord_symbol_index =
      chords_model.index(1, symbol_column, ROOT_INDEX);

  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << first_chord_symbol_index << second_chord_symbol_index;
  QTest::newRow("second chord")
      << second_chord_symbol_index << second_chord_symbol_index;

  auto first_chord_second_note_index =
      chords_model.index(1, symbol_column, first_chord_symbol_index);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index =
      chords_model.index(0, symbol_column, second_chord_symbol_index);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_play() const {
  if (editor_pointer->has_real_time()) {
    editor_pointer->play_selected();
    editor_pointer->stop_playing();
  }
}

void Tester::select_indices(const QModelIndex first_index,
                            const QModelIndex last_index) const {
  auto &selection_model = editor_pointer->get_selection_model();
  selection_model.blockSignals(true);
  selection_model.select(QItemSelection(first_index, last_index),
                         QItemSelectionModel::Current |
                             QItemSelectionModel::Select |
                             QItemSelectionModel::Rows);
  selection_model.blockSignals(false);
}

void Tester::select_index(const QModelIndex index) const {
  select_indices(index, index);
}

void Tester::clear_selection() const {
  editor_pointer->get_selection_model().select(
      QItemSelection(),
      QItemSelectionModel::Current | QItemSelectionModel::Clear);
}

void Tester::test_tree() {
  // test song
  const auto &chords_model = editor_pointer->get_chords_model();
  const auto &first_chord_node =
      *editor_pointer->get_song().root.get_child_pointers()[0];
  auto &first_note_node = *first_chord_node.get_child_pointers()[0];
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto first_note_symbol_index =
      chords_model.index(0, symbol_column, first_chord_symbol_index);

  QCOMPARE(chords_model.rowCount(ROOT_INDEX), 3);
  QCOMPARE(chords_model.columnCount(QModelIndex()), NOTE_CHORD_COLUMNS);
  QCOMPARE(editor_pointer->get_song().root.get_level(), root_level);

  // test first chord
  QCOMPARE(first_chord_node.get_level(), chord_level);
  QCOMPARE(chords_model.parent(first_chord_symbol_index), ROOT_INDEX);
  // only nest the symbol column
  QCOMPARE(
      chords_model.rowCount(chords_model.index(0, interval_column, ROOT_INDEX)),
      0);

  // test first note
  QCOMPARE(chords_model.parent(first_note_symbol_index).row(), 0);
  QCOMPARE(first_note_node.get_level(), note_level);

  QTest::ignoreMessage(QtCriticalMsg, "Notes can't have children!");
  QCOMPARE(first_note_node.get_note_chord().new_child_pointer().get(), nullptr);
}

void Tester::test_set_value_template() {
  QFETCH(QModelIndex, index);
  QFETCH(QVariant, old_value);
  QFETCH(QVariant, old_display_value);
  QFETCH(QVariant, new_value);
  QFETCH(QVariant, new_display_value);

  auto &chords_model = editor_pointer->get_chords_model();

  QVERIFY(chords_model.setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model.data(index, Qt::EditRole), new_value);
  QCOMPARE(chords_model.data(index, Qt::DisplayRole), new_display_value);

  editor_pointer->undo();
  editor_pointer->redo();
  editor_pointer->undo();

  QCOMPARE(chords_model.data(index, Qt::EditRole), old_value);
  QCOMPARE(chords_model.data(index, Qt::DisplayRole), old_display_value);
}

void Tester::test_set_value() {
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto first_note_symbol_index =
      chords_model.index(0, symbol_column, first_chord_symbol_index);

  QVERIFY(chords_model.setData(chords_model.index(0, symbol_column, ROOT_INDEX),
                               QVariant(), Qt::EditRole));

  chords_model.get_node(first_chord_symbol_index)
      .get_note_chord()
      .setData(-1, QVariant());
  // setData only works for the edit role
  QVERIFY(!(chords_model.setData(first_chord_symbol_index, QVariant(),
                                 Qt::DecorationRole)));

  chords_model.get_node(first_note_symbol_index)
      .get_note_chord()
      .setData(-1, QVariant());
}

void Tester::test_set_value_template_data() {
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);

  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("old_display_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_display_value");

  QTest::newRow("first_chord_interval")
      << chords_model.index(0, interval_column, ROOT_INDEX)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_chord_beats")
      << chords_model.index(0, beats_column, ROOT_INDEX) << QVariant(1)
      << QVariant(1) << QVariant(2) << QVariant(2);
  QTest::newRow("first_chord_volume")
      << chords_model.index(0, volume_percent_column, ROOT_INDEX)
      << QVariant::fromValue(PERCENT) << QVariant("100%") << QVariant(2)
      << QVariant("2%");
  QTest::newRow("first_chord_tempo")
      << chords_model.index(0, tempo_percent_column, ROOT_INDEX)
      << QVariant::fromValue(PERCENT) << QVariant("100%") << QVariant(2)
      << QVariant("2%");
  QTest::newRow("first_chord_words")
      << chords_model.index(0, words_column, ROOT_INDEX) << QVariant("")
      << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << chords_model.index(0, instrument_column, ROOT_INDEX)
      << QVariant::fromValue(&Instrument::get_empty_instrument())
      << QVariant("")
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"))
      << QVariant("Oboe");
  QTest::newRow("first_note_interval")
      << chords_model.index(0, interval_column, first_chord_symbol_index)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_note_beats")
      << chords_model.index(0, beats_column, first_chord_symbol_index)
      << QVariant(1) << QVariant(1) << QVariant(2) << QVariant(2);
  QTest::newRow("first_note_volume")
      << chords_model.index(0, volume_percent_column, first_chord_symbol_index)
      << QVariant::fromValue(PERCENT) << QVariant("100%") << QVariant(2)
      << QVariant("2%");
  QTest::newRow("first_note_tempo")
      << chords_model.index(0, tempo_percent_column, first_chord_symbol_index)
      << QVariant::fromValue(PERCENT) << QVariant("100%") << QVariant(2)
      << QVariant("2%");
  QTest::newRow("first_note_words")
      << chords_model.index(0, words_column, first_chord_symbol_index)
      << QVariant("") << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << chords_model.index(0, instrument_column, first_chord_symbol_index)
      << QVariant::fromValue(&Instrument::get_empty_instrument())
      << QVariant("")
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"))
      << QVariant("Oboe");
}

void Tester::test_flags() const {
  // cant edit the symbol
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);

  QCOMPARE(chords_model.flags(first_chord_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(
      chords_model.flags(chords_model.index(0, interval_column, ROOT_INDEX)),
      Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

void Tester::test_get_value() {
  auto &chords_model = editor_pointer->get_chords_model();
  const auto &first_chord_node =
      *editor_pointer->get_song().root.get_child_pointers()[0];
  const auto &first_note_node = *first_chord_node.get_child_pointers()[0];
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto first_note_symbol_index =
      chords_model.index(0, symbol_column, first_chord_symbol_index);

  QCOMPARE(chords_model.data(chords_model.index(0, symbol_column, ROOT_INDEX),
                             Qt::DisplayRole),
           QVariant("♫"));

  // error on non-existent column
  QCOMPARE(first_chord_node.get_const_note_chord().data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non-display data
  QCOMPARE(chords_model.data(first_chord_symbol_index, Qt::DecorationRole),
           QVariant());

  QCOMPARE(chords_model.data(
               chords_model.index(0, symbol_column, first_chord_symbol_index),
               Qt::DisplayRole),
           QVariant("♪"));

  // error on non-existent column
  QCOMPARE(first_note_node.get_const_note_chord().data(-1, Qt::DisplayRole),
           QVariant());
  // empty for non display data
  QCOMPARE(chords_model.data(first_note_symbol_index, Qt::DecorationRole),
           QVariant());

  auto test_interval = Interval();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.get_text(), "1/2");
  test_interval.denominator = DEFAULT_DENOMINATOR;
  test_interval.octave = 1;
  QCOMPARE(test_interval.get_text(), "1o1");
}

void Tester::test_json() {
  editor_pointer->load_text("{");
  auto json_document = editor_pointer->get_song().to_json();
}

void Tester::test_colors_template() {
  QFETCH(QModelIndex, index);
  QFETCH(bool, non_default);

  auto &chords_model = editor_pointer->get_chords_model();
  QCOMPARE(chords_model.data(index, Qt::ForegroundRole),
           non_default ? NON_DEFAULT_COLOR : DEFAULT_COLOR);
}

void Tester::test_colors_template_data() {
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color")
      << chords_model.index(0, symbol_column, ROOT_INDEX) << true;
  QTest::newRow("first_chord_interval_color")
      << chords_model.index(0, interval_column, ROOT_INDEX) << false;
  QTest::newRow("first_chord_beats_color")
      << chords_model.index(0, beats_column, ROOT_INDEX) << false;
  QTest::newRow("first_chord_volume_color")
      << chords_model.index(0, volume_percent_column, ROOT_INDEX) << false;
  QTest::newRow("first_chord_tempo_color")
      << chords_model.index(0, tempo_percent_column, ROOT_INDEX) << false;
  QTest::newRow("first_chord_words_color")
      << chords_model.index(0, words_column, ROOT_INDEX) << false;
  QTest::newRow("first_chord_instrument_color")
      << chords_model.index(0, instrument_column, ROOT_INDEX) << false;

  QTest::newRow("second_chord_interval_color")
      << chords_model.index(1, interval_column, ROOT_INDEX) << true;
  QTest::newRow("second_chord_beats_color")
      << chords_model.index(1, beats_column, ROOT_INDEX) << true;
  QTest::newRow("second_chord_volume_color")
      << chords_model.index(1, volume_percent_column, ROOT_INDEX) << true;
  QTest::newRow("second_chord_tempo_color")
      << chords_model.index(1, tempo_percent_column, ROOT_INDEX) << true;
  QTest::newRow("second_chord_words_color")
      << chords_model.index(1, words_column, ROOT_INDEX) << true;

  QTest::newRow("first_note_symbol_color")
      << chords_model.index(0, symbol_column, first_chord_symbol_index) << true;
  QTest::newRow("first_note_interval_color")
      << chords_model.index(0, interval_column, first_chord_symbol_index)
      << false;
  QTest::newRow("first_note_beats_color")
      << chords_model.index(0, beats_column, first_chord_symbol_index) << false;
  QTest::newRow("first_note_volume_color")
      << chords_model.index(0, volume_percent_column, first_chord_symbol_index)
      << false;
  QTest::newRow("first_note_tempo_color")
      << chords_model.index(0, tempo_percent_column, first_chord_symbol_index)
      << false;
  QTest::newRow("first_note_words_color")
      << chords_model.index(0, words_column, first_chord_symbol_index) << false;
  QTest::newRow("first_note_instrument_color")
      << chords_model.index(0, instrument_column, first_chord_symbol_index)
      << false;

  QTest::newRow("second_note_interval_color")
      << chords_model.index(1, interval_column, first_chord_symbol_index)
      << true;
  QTest::newRow("second_note_beats_color")
      << chords_model.index(1, beats_column, first_chord_symbol_index) << true;
  QTest::newRow("second_note_volume_color")
      << chords_model.index(1, volume_percent_column, first_chord_symbol_index)
      << true;
  QTest::newRow("second_note_tempo_color")
      << chords_model.index(1, tempo_percent_column, first_chord_symbol_index)
      << true;
  QTest::newRow("second_note_words_color")
      << chords_model.index(1, words_column, first_chord_symbol_index) << true;
  QTest::newRow("second_note_instrument_color")
      << chords_model.index(1, instrument_column, first_chord_symbol_index)
      << true;
}

void Tester::test_colors() {
  const auto &first_chord_node =
      *editor_pointer->get_song().root.get_child_pointers()[0];
  const auto &first_note_node = *first_chord_node.get_child_pointers()[0];

  QCOMPARE(first_chord_node.get_const_note_chord().data(-1, Qt::ForegroundRole),
           QVariant());

  // error on non-existent column
  QCOMPARE(first_note_node.get_const_note_chord().data(-1, Qt::ForegroundRole),
           QVariant());
}

void Tester::test_controls_template() const {
  QFETCH(StartingFieldId, value_type);
  QFETCH(QVariant, original_value);
  QFETCH(QVariant, new_value);
  QFETCH(QVariant, new_value_2);

  const auto &song = editor_pointer->get_song();

  auto old_value = editor_pointer->get_starting_control_value(value_type);
  QCOMPARE(old_value, original_value);

  // test change
  editor_pointer->set_starting_control_value(value_type, new_value);
  QCOMPARE(song.get_starting_value(value_type), new_value);
  editor_pointer->undo();
  QCOMPARE(song.get_starting_value(value_type), old_value);

  // test redo
  editor_pointer->redo();
  QCOMPARE(editor_pointer->get_starting_control_value(value_type), new_value);
  editor_pointer->undo();
  QCOMPARE(song.get_starting_value(value_type), original_value);

  // test combining
  editor_pointer->set_starting_control_value(value_type, new_value);
  editor_pointer->set_starting_control_value(value_type, new_value_2);
  QCOMPARE(song.get_starting_value(value_type), new_value_2);
  editor_pointer->undo();
  QCOMPARE(song.get_starting_value(value_type), original_value);
}

void Tester::test_controls_template_data() {
  QTest::addColumn<StartingFieldId>("value_type");
  QTest::addColumn<QVariant>("original_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_value_2");

  QTest::newRow("starting_key")
      << starting_key_id << QVariant::fromValue(ORIGINAL_KEY)
      << QVariant::fromValue(STARTING_KEY_1)
      << QVariant::fromValue(STARTING_KEY_2);
  QTest::newRow("starting_volume")
      << starting_volume_id << QVariant::fromValue(ORIGINAL_VOLUME)
      << QVariant::fromValue(STARTING_VOLUME_1)
      << QVariant::fromValue(STARTING_VOLUME_2);
  QTest::newRow("starting_tempo")
      << starting_tempo_id << QVariant::fromValue(ORIGINAL_TEMPO)
      << QVariant::fromValue(STARTING_TEMPO_1)
      << QVariant::fromValue(STARTING_TEMPO_2);
  QTest::newRow("starting_instrument")
      << starting_instrument_id
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Marimba"))
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"))
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Ocarina"));
}

void Tester::save_to(const QString &filename) const {
  const auto &original_file = editor_pointer->get_current_file();
  editor_pointer->set_current_file(filename);
  editor_pointer->save();
  editor_pointer->set_current_file(original_file);
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
  auto &chords_model = editor_pointer->get_chords_model();
  const auto first_chord_symbol_index =
      chords_model.index(0, symbol_column, ROOT_INDEX);
  const auto second_chord_symbol_index =
      chords_model.index(1, symbol_column, ROOT_INDEX);
  auto item_selection =
      QItemSelection(first_chord_symbol_index, second_chord_symbol_index);
  editor_pointer->get_selection_model().select(item_selection,
                                               QItemSelectionModel::Select);
  editor_pointer->get_selection_model().select(item_selection,
                                               QItemSelectionModel::Deselect);
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
  editor_pointer->export_recording_file(temp_json_file.fileName());

  save_to("/<>:\"/\\|?*");
  editor_pointer->save_as_file("/<>:\"/\\|?*");
  editor_pointer->open_file("/<>:\"/\\|?*");
}

void Tester::test_delegate_template() {
  QFETCH(QModelIndex, index);
  QFETCH(QVariant, old_value);
  QFETCH(QVariant, new_value);

  auto &chords_model = editor_pointer->get_chords_model();

  const auto &my_delegate = editor_pointer->get_delegate();

  auto *cell_editor_pointer =
      my_delegate.createEditor(editor_pointer->get_chords_viewport_pointer(),
                               QStyleOptionViewItem(), index);

  my_delegate.updateEditorGeometry(cell_editor_pointer, QStyleOptionViewItem(),
                                   index);

  my_delegate.setEditorData(cell_editor_pointer, index);

  QCOMPARE(cell_editor_pointer->size(), cell_editor_pointer->sizeHint());

  QVariant current_value;
  auto column = index.column();
  if (column == beats_column) {
    current_value = qobject_cast<QSpinBox *>(cell_editor_pointer)->value();
  }
  if (column == interval_column) {
    current_value = QVariant::fromValue(qobject_cast<IntervalEditor *>(cell_editor_pointer)->value());
  }
  if (column == volume_percent_column || column == tempo_percent_column) {
    current_value = qobject_cast<QDoubleSpinBox *>(cell_editor_pointer)->value();
  }
  if (column == instrument_column) {
    current_value = QVariant::fromValue(qobject_cast<InstrumentEditor *>(cell_editor_pointer)->value());
  }

  QCOMPARE(old_value, current_value);

  if (column == beats_column) {
    qobject_cast<QSpinBox *>(cell_editor_pointer)->setValue(new_value.toInt());
  } else if (column == interval_column) {
    qobject_cast<IntervalEditor *>(cell_editor_pointer)->setValue(new_value.value<Interval>());
  } else if (column == volume_percent_column ||
             column == tempo_percent_column) {
    qobject_cast<QDoubleSpinBox *>(cell_editor_pointer)->setValue(new_value.toDouble());
  } else if (column == instrument_column) {
    qobject_cast<InstrumentEditor *>(cell_editor_pointer)->setValue(new_value.value<const Instrument *>());
  }
  my_delegate.setModelData(cell_editor_pointer, &chords_model, index);

  QCOMPARE(chords_model.data(index, Qt::EditRole), new_value);
  editor_pointer->undo();
  QCOMPARE(chords_model.data(index, Qt::EditRole), old_value);
}

void Tester::test_delegate_template_data() {
  auto &chords_model = editor_pointer->get_chords_model();

  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("interval header")
      << chords_model.index(0, interval_column, ROOT_INDEX)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats header")
      << chords_model.index(0, beats_column, ROOT_INDEX) << QVariant(1)
      << QVariant(2);
  QTest::newRow("volume header")
      << chords_model.index(0, volume_percent_column, ROOT_INDEX)
      << QVariant(PERCENT) << QVariant(VOLUME_PERCENT_1);
  QTest::newRow("tempo header")
      << chords_model.index(0, tempo_percent_column, ROOT_INDEX)
      << QVariant(PERCENT) << QVariant(TEMPO_PERCENT_1);
  QTest::newRow("instruments header")
      << chords_model.index(0, instrument_column, ROOT_INDEX)
      << QVariant::fromValue(&Instrument::get_instrument_by_name(""))
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"));
}