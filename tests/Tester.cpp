#include "tests/Tester.h"

#include <qabstractitemdelegate.h>  // for QAbstractItemDelegate
#include <qabstractitemmodel.h>     // for QModelIndex, QModelIndexList
#include <qabstractitemview.h>      // for QAbstractItemView
#include <qapplication.h>           // for QApplication
#include <qglobal.h>                // for QFlags, QtCriticalMsg
#include <qitemselectionmodel.h>    // for QItemSelectionModel, operator|
#include <qlist.h>                  // for QList<>::iterator, QList
#include <qmessagebox.h>            // for QMessageBox
#include <qnamespace.h>             // for DisplayRole, operator|, Decoration...
#include <qobject.h>                // for qobject_cast
#include <qspinbox.h>
#include <qstyleoption.h>    // for QStyleOptionViewItem
#include <qtemporaryfile.h>  // for QTemporaryFile
#include <qtest.h>           // for qCompare
#include <qtestcase.h>       // for newRow, qCompare, addColumn, QCOMPARE
#include <qtestdata.h>       // for operator<<, QTestData
#include <qtestkeyboard.h>   // for keyEvent, Press
#include <qthread.h>
#include <qtimer.h>       // for QTimer
#include <qvariant.h>     // for QVariant
#include <qwidget.h>      // for QWidget
#include <qwindowdefs.h>  // for QWidgetList

#include <memory>       // for unique_ptr, allocator, make_unique
#include <type_traits>  // for enable_if_t

#include "src/ChordsModel.h"  // for DEFAULT_COLOR, NON_DEFAULT_COLOR
#include "src/Instrument.h"   // for Instrument
#include "src/InstrumentEditor.h"
#include "justly/Interval.h"    // for Interval, DEFAULT_DENOMINATOR
#include "justly/SongEditor.h"  // for SongEditor
#include "justly/StartingField.h"
#include "src/IntervalEditor.h"

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

const auto NEW_PERCENT = 101.0;

const auto NO_DATA = QVariant();

const auto WAIT_TIME = 1000;

auto Tester::get_index(int chord_number, int note_number,
                       NoteChordField column_number) const -> QModelIndex {
  auto *chords_model_pointer = song_editor.get_chords_model_pointer();
  auto root_index = QModelIndex();
  if (chord_number == -1) {
    return root_index;
  }
  if (note_number == -1) {
    return chords_model_pointer->index(chord_number, column_number, root_index);
  }
  return chords_model_pointer->index(
      note_number, column_number,
      chords_model_pointer->index(chord_number, symbol_column, root_index));
}

void Tester::initTestCase() {
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &Tester::close_message);
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
  song_editor.open_file(main_file.fileName());

  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  QCOMPARE(song_editor.get_number_of_children(1), 1);
}

void Tester::test_column_headers_template() const {
  QFETCH(const NoteChordField, field);
  QFETCH(const QVariant, value);

  QCOMPARE(song_editor.get_chords_model_pointer()->headerData(
               field, Qt::Horizontal, Qt::DisplayRole),
           value);
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
  auto *chords_model_pointer = song_editor.get_chords_model_pointer();
  // no vertical labels
  QCOMPARE(chords_model_pointer->headerData(interval_column, Qt::Vertical,
                                            Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(chords_model_pointer->headerData(interval_column, Qt::Horizontal,
                                            Qt::DecorationRole),
           QVariant());
}

void Tester::test_insert_delete() {
  select_index(get_index(2));
  song_editor.insert_into();
  QCOMPARE(song_editor.get_number_of_children(2), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(2), 0);
  clear_selection();

  select_index(get_index(0, 0));
  song_editor.insert_before();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  clear_selection();
  song_editor.insert_before();

  select_index(get_index(0, 0));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  clear_selection();
  song_editor.insert_after();

  select_index(get_index(0, 0));
  song_editor.remove_selected();
  QCOMPARE(song_editor.get_number_of_children(0), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  clear_selection();
  song_editor.remove_selected();

  select_index(get_index(0));
  song_editor.insert_before();

  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  clear_selection();
  song_editor.insert_before();

  select_index(get_index(0));
  song_editor.remove_selected();
  QCOMPARE(song_editor.get_number_of_children(-1), 2);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  clear_selection();
  song_editor.remove_selected();

  select_index(get_index(0));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  clear_selection();
  QCOMPARE(song_editor.get_selected_rows().size(), 0);
  song_editor.insert_after();

  // need to do after because these gets invalidated by removals
}

void Tester::test_copy_paste() {
  select_index(get_index(0));
  song_editor.copy_selected();
  clear_selection();
  song_editor.copy_selected();

  // paste after first chord
  select_index(get_index(0));
  song_editor.paste_before();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  clear_selection();
  song_editor.paste_before();

  select_index(get_index(0));
  song_editor.paste_after();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  clear_selection();
  song_editor.paste_after();

  select_index(get_index(0, 0));
  song_editor.copy_selected();
  clear_selection();
  song_editor.copy_selected();

  select_index(get_index(0, 0));
  song_editor.paste_before();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  clear_selection();
  song_editor.paste_before();

  select_index(get_index(0, 0));
  song_editor.paste_after();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  clear_selection();
  song_editor.paste_after();

  select_index(get_index(2));
  song_editor.paste_into();
  QCOMPARE(song_editor.get_number_of_children(2), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(2), 0);
  clear_selection();

  song_editor.paste_text(0, "[", get_index());

  song_editor.paste_text(0, "{}", get_index());

  song_editor.paste_text(0, "[", get_index(0));

  song_editor.paste_text(0, "{}", get_index(0));
}

void Tester::test_play_template() {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  select_indices(first_index, last_index);
  // use the second chord to test key changing
  song_editor.play_selected();
  // first cut off early
  song_editor.play_selected();
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  clear_selection();
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords") << get_index(0) << get_index(1);
  QTest::newRow("second chord") << get_index(1) << get_index(1);

  auto first_chord_second_note_index = get_index(0, 1);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index = get_index(1, 0);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_play() const {
  song_editor.play_selected();
  song_editor.stop();
}

void Tester::select_indices(const QModelIndex first_index,
                            const QModelIndex last_index) {
  song_editor.get_chords_view_pointer()->selectionModel()->select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void Tester::select_index(const QModelIndex index) {
  select_indices(index, index);
}

void Tester::clear_selection() {
  song_editor.get_chords_view_pointer()->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);
}

void Tester::test_tree() {
  // test song
  auto *const chords_model_pointer = song_editor.get_chords_model_pointer();
  QCOMPARE(chords_model_pointer->rowCount(get_index()), 3);
  QCOMPARE(chords_model_pointer->columnCount(QModelIndex()),
           NOTE_CHORD_COLUMNS);

  QCOMPARE(chords_model_pointer->parent(get_index(0)), get_index());
  // only nest the symbol column
  QCOMPARE(chords_model_pointer->rowCount(get_index(0, -1, interval_column)),
           0);

  // test first note
  QCOMPARE(chords_model_pointer->parent(get_index(0, 0)).row(), 0);
}

void Tester::test_set_value_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, old_display_value);
  QFETCH(const QVariant, new_value);
  QFETCH(const QVariant, new_display_value);

  auto *chords_model_pointer = song_editor.get_chords_model_pointer();

  QVERIFY(chords_model_pointer->setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole),
           new_display_value);

  song_editor.undo();
  song_editor.redo();
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole),
           old_display_value);
}

void Tester::test_set_value() {
  auto *chords_model_pointer = song_editor.get_chords_model_pointer();

  QVERIFY(
      chords_model_pointer->setData(get_index(0), QVariant(), Qt::EditRole));
  // setData only works for the edit role
  QVERIFY(!(chords_model_pointer->setData(get_index(0), QVariant(),
                                          Qt::DecorationRole)));
}

void Tester::test_set_value_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("old_display_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_display_value");

  QTest::newRow("first_chord_interval")
      << get_index(0, -1, interval_column) << QVariant::fromValue(Interval())
      << QVariant("1") << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_chord_beats")
      << get_index(0, -1, beats_column) << QVariant(1) << QVariant(1)
      << QVariant(2) << QVariant(2);
  QTest::newRow("first_chord_volume")
      << get_index(0, -1, volume_percent_column) << QVariant::fromValue(PERCENT)
      << QVariant("100%") << QVariant(2) << QVariant("2%");
  QTest::newRow("first_chord_tempo")
      << get_index(0, -1, tempo_percent_column) << QVariant::fromValue(PERCENT)
      << QVariant("100%") << QVariant(2) << QVariant("2%");
  QTest::newRow("first_chord_words")
      << get_index(0, -1, words_column) << QVariant("") << QVariant("")
      << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << get_index(0, -1, instrument_column)
      << QVariant::fromValue(&Instrument::get_empty_instrument())
      << QVariant("")
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"))
      << QVariant("Oboe");
  QTest::newRow("first_note_interval")
      << get_index(0, 0, interval_column) << QVariant::fromValue(Interval())
      << QVariant("1") << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_note_beats")
      << get_index(0, 0, beats_column) << QVariant(1) << QVariant(1)
      << QVariant(2) << QVariant(2);
  QTest::newRow("first_note_volume")
      << get_index(0, 0, volume_percent_column) << QVariant::fromValue(PERCENT)
      << QVariant("100%") << QVariant(2) << QVariant("2%");
  QTest::newRow("first_note_tempo")
      << get_index(0, 0, tempo_percent_column) << QVariant::fromValue(PERCENT)
      << QVariant("100%") << QVariant(2) << QVariant("2%");
  QTest::newRow("first_note_words")
      << get_index(0, 0, words_column) << QVariant("") << QVariant("")
      << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << get_index(0, 0, instrument_column)
      << QVariant::fromValue(&Instrument::get_empty_instrument())
      << QVariant("")
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"))
      << QVariant("Oboe");
}

void Tester::test_flags() const {
  // cant edit the symbol
  auto *chords_model_pointer = song_editor.get_chords_model_pointer();

  QCOMPARE(chords_model_pointer->flags(get_index(0)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(chords_model_pointer->flags(get_index(0, -1, interval_column)),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

void Tester::test_get_value() {
  auto *chords_model_pointer = song_editor.get_chords_model_pointer();

  QCOMPARE(chords_model_pointer->data(get_index(0), Qt::DisplayRole),
           QVariant("♫"));
  // empty for non-display data
  QCOMPARE(chords_model_pointer->data(get_index(0), Qt::DecorationRole),
           QVariant());

  QCOMPARE(chords_model_pointer->data(get_index(0, 0), Qt::DisplayRole),
           QVariant("♪"));

  // empty for non display data
  QCOMPARE(chords_model_pointer->data(get_index(0, 0), Qt::DecorationRole),
           QVariant());

  QCOMPARE(chords_model_pointer->data(get_index(0, 1, interval_column),
                                      Qt::DisplayRole),
           QVariant("2/2o1"));

  auto test_interval = Interval();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.text(), "1/2");
  test_interval.denominator = DEFAULT_DENOMINATOR;
  test_interval.octave = 1;
  QCOMPARE(test_interval.text(), "1o1");
}

void Tester::test_colors_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const bool, non_default);

  auto *chords_model_pointer = song_editor.get_chords_model_pointer();
  QCOMPARE(chords_model_pointer->data(index, Qt::ForegroundRole),
           non_default ? NON_DEFAULT_COLOR : DEFAULT_COLOR);
}

void Tester::test_colors_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color") << get_index(0) << true;
  QTest::newRow("first_chord_interval_color")
      << get_index(0, -1, interval_column) << false;
  QTest::newRow("first_chord_beats_color")
      << get_index(0, -1, beats_column) << false;
  QTest::newRow("first_chord_volume_color")
      << get_index(0, -1, volume_percent_column) << false;
  QTest::newRow("first_chord_tempo_color")
      << get_index(0, -1, tempo_percent_column) << false;
  QTest::newRow("first_chord_words_color")
      << get_index(0, -1, words_column) << false;
  QTest::newRow("first_chord_instrument_color")
      << get_index(0, -1, instrument_column) << false;

  QTest::newRow("second_chord_interval_color")
      << get_index(1, -1, interval_column) << true;
  QTest::newRow("second_chord_beats_color")
      << get_index(1, -1, beats_column) << true;
  QTest::newRow("second_chord_volume_color")
      << get_index(1, -1, volume_percent_column) << true;
  QTest::newRow("second_chord_tempo_color")
      << get_index(1, -1, tempo_percent_column) << true;
  QTest::newRow("second_chord_words_color")
      << get_index(1, -1, words_column) << true;

  QTest::newRow("first_note_symbol_color") << get_index(0, 0) << true;
  QTest::newRow("first_note_interval_color")
      << get_index(0, 0, interval_column) << false;
  QTest::newRow("first_note_beats_color")
      << get_index(0, 0, beats_column) << false;
  QTest::newRow("first_note_volume_color")
      << get_index(0, 0, volume_percent_column) << false;
  QTest::newRow("first_note_tempo_color")
      << get_index(0, 0, tempo_percent_column) << false;
  QTest::newRow("first_note_words_color")
      << get_index(0, 0, words_column) << false;
  QTest::newRow("first_note_instrument_color")
      << get_index(0, 0, instrument_column) << false;

  QTest::newRow("second_note_interval_color")
      << get_index(0, 1, interval_column) << true;
  QTest::newRow("second_note_beats_color")
      << get_index(0, 1, beats_column) << true;
  QTest::newRow("second_note_volume_color")
      << get_index(0, 1, volume_percent_column) << true;
  QTest::newRow("second_note_tempo_color")
      << get_index(0, 1, tempo_percent_column) << true;
  QTest::newRow("second_note_words_color")
      << get_index(0, 1, words_column) << true;
  QTest::newRow("second_note_instrument_color")
      << get_index(0, 1, instrument_column) << true;
}

void Tester::test_controls_template() {
  QFETCH(const StartingField, value_type);
  QFETCH(const QVariant, original_value);
  QFETCH(const QVariant, new_value);
  QFETCH(const QVariant, new_value_2);

  auto old_value = song_editor.get_starting_value(value_type);
  QCOMPARE(old_value, original_value);

  // test change
  song_editor.set_starting_control(value_type, new_value);
  QCOMPARE(song_editor.get_starting_value(value_type), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_value(value_type), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_starting_value(value_type), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_value(value_type), original_value);

  // test combining
  song_editor.set_starting_control(value_type, new_value);
  song_editor.set_starting_control(value_type, new_value_2);
  QCOMPARE(song_editor.get_starting_value(value_type), new_value_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_value(value_type), original_value);
}

void Tester::test_controls_template_data() {
  QTest::addColumn<StartingField>("value_type");
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

void Tester::close_message() {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *box_pointer = qobject_cast<QMessageBox *>(widget_pointer);
    if (box_pointer != nullptr) {
      QTest::keyEvent(QTest::Press, qobject_cast<QMessageBox *>(box_pointer),
                      Qt::Key_Enter);
      return;
    }
  }
}

void Tester::test_io() {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  song_editor.save_as_file(temp_json_file.fileName());
  QCOMPARE(song_editor.get_current_file(), temp_json_file.fileName());
  song_editor.save();

  const QTemporaryFile temp_wav_file;
  temp_json_file.open();
  temp_json_file.close();
  song_editor.export_to(temp_json_file.fileName());

  const QTemporaryFile broken_json_file;
  temp_json_file.open();
  temp_json_file.write("{");
  temp_json_file.close();
  song_editor.open_file(broken_json_file.fileName());
}

void Tester::test_delegate_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  auto *chords_model_pointer = song_editor.get_chords_model_pointer();
  auto *chords_view_pointer = song_editor.get_chords_view_pointer();

  auto *const my_delegate_pointer = chords_view_pointer->itemDelegate();

  auto *cell_editor_pointer = my_delegate_pointer->createEditor(
      chords_view_pointer->viewport(), QStyleOptionViewItem(), index);

  my_delegate_pointer->setEditorData(cell_editor_pointer, index);

  auto column = static_cast<NoteChordField>(index.column());
  switch (column) {
    case beats_column: {
      QCOMPARE(old_value,
               QVariant::fromValue(
                   qobject_cast<QSpinBox *>(cell_editor_pointer)->value()));
      qobject_cast<QSpinBox *>(cell_editor_pointer)
          ->setValue(new_value.toInt());
      break;
    }
    case interval_column: {
      QCOMPARE(old_value, QVariant::fromValue(qobject_cast<IntervalEditor *>(
                                                  cell_editor_pointer)
                                                  ->get_interval()));
      qobject_cast<IntervalEditor *>(cell_editor_pointer)
          ->set_interval(new_value.value<Interval>());
      break;
    }
    case instrument_column: {
      QCOMPARE(old_value, QVariant::fromValue(qobject_cast<InstrumentEditor *>(
                                                  cell_editor_pointer)
                                                  ->get_instrument_pointer()));
      qobject_cast<InstrumentEditor *>(cell_editor_pointer)
          ->set_instrument_pointer(new_value.value<const Instrument *>());
      break;
    }
    default:  // volume_percent_column, tempo_percent_column
      QCOMPARE(old_value,
               qobject_cast<QDoubleSpinBox *>(cell_editor_pointer)->value());
      qobject_cast<QDoubleSpinBox *>(cell_editor_pointer)
          ->setValue(new_value.toDouble());
      break;
  }

  my_delegate_pointer->setModelData(cell_editor_pointer, chords_model_pointer,
                                    index);

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
}

void Tester::test_delegate_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("beats song_editor")
      << get_index(0, -1, beats_column) << QVariant(1) << QVariant(2);
  QTest::newRow("interval song_editor")
      << get_index(0, -1, interval_column) << QVariant::fromValue(Interval(1))
      << QVariant::fromValue(Interval(2));
  QTest::newRow("volume song_editor")
      << get_index(0, -1, volume_percent_column) << QVariant(PERCENT)
      << QVariant(NEW_PERCENT);
  QTest::newRow("tempo song_editor")
      << get_index(0, -1, tempo_percent_column) << QVariant(PERCENT)
      << QVariant(NEW_PERCENT);
  QTest::newRow("instrument song_editor")
      << get_index(0, -1, instrument_column)
      << QVariant::fromValue(&Instrument::get_instrument_by_name(""))
      << QVariant::fromValue(&Instrument::get_instrument_by_name("Oboe"));
}

void Tester::test_select() {
  select_index(get_index(0));
  select_index(get_index(0, 0));
  auto selected_rows = song_editor.get_selected_rows();
  QCOMPARE(selected_rows.size(), 1);
  QCOMPARE(selected_rows[0], get_index(0));
  clear_selection();

  select_index(get_index(0, 0));
  select_index(get_index(0));
  auto selected_rows_2 = song_editor.get_selected_rows();
  QCOMPARE(selected_rows_2.size(), 1);
  QCOMPARE(selected_rows_2[0], get_index(0, 0));
  clear_selection();
}
