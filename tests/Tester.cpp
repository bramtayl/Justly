#include "tests/Tester.h"

#include <qabstractitemmodel.h>   // for QModelIndex
#include <qaction.h>              // for QAction
#include <qapplication.h>         // for QApplication
#include <qdebug.h>               // for operator<<
#include <qflags.h>               // for QFlags, operator==
#include <qitemselectionmodel.h>  // for QItemSelectionModel
#include <qlist.h>                // for QList, QList<>::ite...
#include <qmessagebox.h>          // for QMessageBox
#include <qmetaobject.h>          // for QMetaProperty
#include <qnamespace.h>           // for qt_getEnumName, Ite...
#include <qobjectdefs.h>          // for QMetaObject
#include <qslider.h>              // for QSlider
#include <qspinbox.h>             // for QDoubleSpinBox
#include <qstring.h>              // for QString
#include <qtemporaryfile.h>       // for QTemporaryFile
#include <qtest.h>                // for qCompare
#include <qtestcase.h>            // for newRow, qCompare
#include <qtestdata.h>            // for operator<<, QTestData
#include <qtestkeyboard.h>        // for keyEvent, Press
#include <qthread.h>              // for QThread
#include <qtimer.h>               // for QTimer
#include <qundostack.h>
#include <qvariant.h>     // for QVariant
#include <qwidget.h>      // for QWidget
#include <qwindowdefs.h>  // for QWidgetList

#include <map>                    // for operator!=
#include <memory>                 // for allocator, make_unique
#include <nlohmann/json.hpp>      // for basic_json
#include <nlohmann/json_fwd.hpp>  // for json
#include <type_traits>            // for enable_if_t

#include "justly/ChordsModel.hpp"       // for ChordsModel
#include "justly/ChordsView.hpp"        // for ChordsView
#include "justly/Instrument.hpp"        // for get_instrument_pointer
#include "justly/InstrumentEditor.hpp"  // for InstrumentEditor
#include "justly/Interval.hpp"          // for Interval
#include "justly/NoteChordField.hpp"    // for NoteChordField, int...
#include "justly/Rational.hpp"          // for Rational
#include "justly/SongEditor.hpp"        // for SongEditor
#include "justly/public_constants.hpp"  // for DEFAULT_COLOR, NON_...

const auto ORIGINAL_KEY = 220.0;
const auto STARTING_KEY_1 = 401.0;
const auto STARTING_KEY_2 = 402.0;
const auto ORIGINAL_TEMPO = 200.0;
const auto STARTING_TEMPO_1 = 150.0;
const auto STARTING_TEMPO_2 = 100.0;
const auto ORIGINAL_VOLUME = 50.0;
const auto STARTING_VOLUME_1 = 51.0;
const auto STARTING_VOLUME_2 = 52.0;

const auto WAIT_TIME = 1000;

const auto OVERLOAD_NUMBER = 15;

const auto NEW_VOLUME_PERCENT = 100;

const auto SELECT_ROWS =
    QItemSelectionModel::Select | QItemSelectionModel::Rows;

const auto SELECT_CELL = QFlags(QItemSelectionModel::Select);

// TODO: check warning message
void Tester::close_message_later(std::string expected_text) {
  auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  QTimer *timer_pointer = std::make_unique<QTimer>(this).release();
  timer_pointer->setSingleShot(true);
  connect(timer_pointer, &QTimer::timeout, this, [this, expected_text]() {
    for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
      auto *box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
      if (box_pointer != nullptr) {
        auto actual_text = box_pointer->text().toStdString();
        waiting_for_message = false;
        QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
        QCOMPARE(actual_text, expected_text);
        break;
      }
    }
  });
  timer_pointer->start(WAIT_TIME);
  QVERIFY(!waiting_before);
}

void Tester::clear_selection() const {
  selector_pointer->select(QModelIndex(), QItemSelectionModel::Clear);
}

void Tester::trigger_action(const QModelIndex &index,
                            QItemSelectionModel::SelectionFlags flags,
                            QAction *action_pointer) const {
  selector_pointer->select(index, flags);
  action_pointer->trigger();
  clear_selection();
}

void Tester::initTestCase() {
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
                    "beats": {
                        "numerator": 2
                    },
                    "volume_ratio": {
                        "numerator": 2
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
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
            "beats": {
                "numerator": 2
            },
            "volume_ratio": {
                "numerator": 2
            },
            "tempo_ratio": {
                "numerator": 2
            },
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
  song_editor.open_file(main_file.fileName().toStdString());
}

void Tester::test_interval() {
  auto test_interval = Interval();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.text(), "1/2");
  test_interval.denominator = 1;
  test_interval.octave = 1;
  QCOMPARE(test_interval.text(), "1o1");
}

void Tester::test_rational() {
  auto test_interval = Rational();
  test_interval.denominator = 2;
  QCOMPARE(test_interval.text(), "1/2");
  QVERIFY(test_interval.json().contains("denominator"));
}

void Tester::test_row_count_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(int, row_count);

  QCOMPARE(chords_model_pointer->rowCount(index), row_count);
}

void Tester::test_row_count_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("row_count");

  QTest::newRow("song") << QModelIndex() << 3;
  QTest::newRow("first chord") << chords_model_pointer->get_index(0) << 2;
  QTest::newRow("second chord") << chords_model_pointer->get_index(1) << 1;
  QTest::newRow("non-symbol chord")
      << chords_model_pointer->get_index(0, -1, interval_column) << 0;
}

void Tester::test_tree() const {
  // test song
  QCOMPARE(chords_model_pointer->columnCount(QModelIndex()),
           NOTE_CHORD_COLUMNS);

  QCOMPARE(chords_model_pointer->parent(chords_model_pointer->get_index(0)),
           QModelIndex());
  // only nest the symbol column
  QCOMPARE(chords_model_pointer->rowCount(
               chords_model_pointer->get_index(0, -1, interval_column)),
           0);

  // test first note
  QCOMPARE(
      chords_model_pointer->parent(chords_model_pointer->get_index(0, 0)).row(),
      0);
}

void Tester::test_playback_volume_control() {
  auto old_playback_volume = song_editor.get_playback_volume();
  song_editor.playback_volume_editor_pointer->setValue(NEW_VOLUME_PERCENT);
  QCOMPARE(song_editor.get_playback_volume(),
           NEW_VOLUME_PERCENT / PERCENT * MAX_GAIN);
  song_editor.playback_volume_editor_pointer->setValue(old_playback_volume *
                                                       PERCENT / MAX_GAIN);
}

void Tester::test_starting_instrument_control() const {
  const auto *original_value = get_instrument_pointer("Marimba");
  const auto *new_value = get_instrument_pointer("Oboe");
  const auto *new_value_2 = get_instrument_pointer("Ocarina");

  const auto *old_value = song_editor.starting_instrument_pointer;
  QCOMPARE(old_value, get_instrument_pointer("Marimba"));

  // test change
  starting_instrument_editor_pointer->setValue(new_value);
  QCOMPARE(song_editor.starting_instrument_pointer, new_value);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer, old_value);

  // test redo
  undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_instrument_pointer, new_value);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer, original_value);

  // test combining
  starting_instrument_editor_pointer->setValue(new_value);
  starting_instrument_editor_pointer->setValue(new_value_2);
  QCOMPARE(song_editor.starting_instrument_pointer, new_value_2);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer, original_value);
}

void Tester::test_starting_key_control() const {
  auto old_value = song_editor.starting_key;
  QCOMPARE(old_value, ORIGINAL_KEY);

  // test change
  starting_key_editor_pointer->setValue(STARTING_KEY_1);
  QCOMPARE(song_editor.starting_key, STARTING_KEY_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, old_value);

  // test redo
  undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_key, STARTING_KEY_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, ORIGINAL_KEY);

  // test combining
  starting_key_editor_pointer->setValue(STARTING_KEY_1);
  starting_key_editor_pointer->setValue(STARTING_KEY_2);
  QCOMPARE(song_editor.starting_key, STARTING_KEY_2);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, ORIGINAL_KEY);
}

void Tester::test_starting_volume_control() const {
  auto old_value = song_editor.starting_volume_percent;
  QCOMPARE(old_value, ORIGINAL_VOLUME);

  // test change
  starting_volume_editor_pointer->setValue(STARTING_VOLUME_1);
  QCOMPARE(song_editor.starting_volume_percent, STARTING_VOLUME_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, old_value);

  // test redo
  undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_volume_percent, STARTING_VOLUME_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, ORIGINAL_VOLUME);

  // test combining
  starting_volume_editor_pointer->setValue(STARTING_VOLUME_1);
  starting_volume_editor_pointer->setValue(STARTING_VOLUME_2);
  QCOMPARE(song_editor.starting_volume_percent, STARTING_VOLUME_2);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, ORIGINAL_VOLUME);
}

void Tester::test_starting_tempo_control() const {
  auto old_value = song_editor.starting_tempo;
  QCOMPARE(old_value, ORIGINAL_TEMPO);

  // test change
  starting_tempo_editor_pointer->setValue(STARTING_TEMPO_1);
  QCOMPARE(song_editor.starting_tempo, STARTING_TEMPO_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, old_value);

  // test redo
  undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_tempo, STARTING_TEMPO_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, ORIGINAL_TEMPO);

  // test combining
  starting_tempo_editor_pointer->setValue(STARTING_TEMPO_1);
  starting_tempo_editor_pointer->setValue(STARTING_TEMPO_2);
  QCOMPARE(song_editor.starting_tempo, STARTING_TEMPO_2);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, ORIGINAL_TEMPO);
}

void Tester::test_column_headers_template() const {
  QFETCH(const NoteChordField, field);
  QFETCH(const QVariant, value);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);

  QCOMPARE(chords_model_pointer->headerData(field, orientation, role), value);
}

void Tester::test_column_headers_template_data() {
  QTest::addColumn<NoteChordField>("field");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("symbol header")
      << symbol_column << Qt::Horizontal << Qt::DisplayRole << QVariant();
  QTest::newRow("interval header") << interval_column << Qt::Horizontal
                                   << Qt::DisplayRole << QVariant("Interval");
  QTest::newRow("beats header")
      << beats_column << Qt::Horizontal << Qt::DisplayRole << QVariant("Beats");
  QTest::newRow("volume header") << volume_ratio_column << Qt::Horizontal
                                 << Qt::DisplayRole << QVariant("Volume ratio");
  QTest::newRow("tempo header") << tempo_ratio_column << Qt::Horizontal
                                << Qt::DisplayRole << QVariant("Tempo ratio");
  QTest::newRow("words header")
      << words_column << Qt::Horizontal << Qt::DisplayRole << QVariant("Words");
  QTest::newRow("instruments header")
      << instrument_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant("Instrument");
  QTest::newRow("horizontal labels")
      << symbol_column << Qt::Horizontal << Qt::DisplayRole << QVariant();
  QTest::newRow("wrong role")
      << symbol_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
  // QTest::newRow("non-existent column")
  //    << -1 << Qt::Horizontal << Qt::DecorationRole << QVariant();
}

void Tester::test_select_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, second_index);
  selector_pointer->select(first_index, SELECT_ROWS);
  selector_pointer->select(second_index, SELECT_ROWS);
  auto selected_rows = selector_pointer->selectedRows();
  QCOMPARE(selected_rows.size(), 1);
  QCOMPARE(selected_rows[0], first_index);
  clear_selection();
}

void Tester::test_select_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("second_index");

  QTest::newRow("select_chord_then_note")
      << chords_model_pointer->get_index(0)
      << chords_model_pointer->get_index(0, 0);
  QTest::newRow("select_note_then_chord")
      << chords_model_pointer->get_index(0, 0)
      << chords_model_pointer->get_index(0);
}

void Tester::test_flags_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemFlags, item_flags);
  QCOMPARE(chords_model_pointer->flags(index), item_flags);
}

void Tester::test_flags_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemFlags>("item_flags");

  QTest::newRow("first_chord_symbol_flag")
      << chords_model_pointer->get_index(0)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first_chord_interval_flag")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

void Tester::test_colors_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const bool, non_default);
  QCOMPARE(chords_model_pointer->data(index, Qt::ForegroundRole),
           non_default ? NON_DEFAULT_COLOR : DEFAULT_COLOR);
}

void Tester::test_colors_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color")
      << chords_model_pointer->get_index(0) << true;
  QTest::newRow("first_chord_interval_color")
      << chords_model_pointer->get_index(0, -1, interval_column) << false;
  QTest::newRow("first_chord_beats_color")
      << chords_model_pointer->get_index(0, -1, beats_column) << false;
  QTest::newRow("first_chord_volume_color")
      << chords_model_pointer->get_index(0, -1, volume_ratio_column) << false;
  QTest::newRow("first_chord_tempo_color")
      << chords_model_pointer->get_index(0, -1, tempo_ratio_column) << false;
  QTest::newRow("first_chord_words_color")
      << chords_model_pointer->get_index(0, -1, words_column) << false;
  QTest::newRow("first_chord_instrument_color")
      << chords_model_pointer->get_index(0, -1, instrument_column) << false;

  QTest::newRow("second_chord_interval_color")
      << chords_model_pointer->get_index(1, -1, interval_column) << true;
  QTest::newRow("second_chord_beats_color")
      << chords_model_pointer->get_index(1, -1, beats_column) << true;
  QTest::newRow("second_chord_volume_color")
      << chords_model_pointer->get_index(1, -1, volume_ratio_column) << true;
  QTest::newRow("second_chord_tempo_color")
      << chords_model_pointer->get_index(1, -1, tempo_ratio_column) << true;
  QTest::newRow("second_chord_words_color")
      << chords_model_pointer->get_index(1, -1, words_column) << true;

  QTest::newRow("first_note_symbol_color")
      << chords_model_pointer->get_index(0, 0) << true;
  QTest::newRow("first_note_interval_color")
      << chords_model_pointer->get_index(0, 0, interval_column) << false;
  QTest::newRow("first_note_beats_color")
      << chords_model_pointer->get_index(0, 0, beats_column) << false;
  QTest::newRow("first_note_volume_color")
      << chords_model_pointer->get_index(0, 0, volume_ratio_column) << false;
  QTest::newRow("first_note_tempo_color")
      << chords_model_pointer->get_index(0, 0, tempo_ratio_column) << false;
  QTest::newRow("first_note_words_color")
      << chords_model_pointer->get_index(0, 0, words_column) << false;
  QTest::newRow("first_note_instrument_color")
      << chords_model_pointer->get_index(0, 0, instrument_column) << false;

  QTest::newRow("second_note_interval_color")
      << chords_model_pointer->get_index(1, 0, interval_column) << true;
  QTest::newRow("second_note_beats_color")
      << chords_model_pointer->get_index(1, 0, beats_column) << true;
  QTest::newRow("second_note_volume_color")
      << chords_model_pointer->get_index(1, 0, volume_ratio_column) << true;
  QTest::newRow("second_note_tempo_color")
      << chords_model_pointer->get_index(1, 0, tempo_ratio_column) << true;
  QTest::newRow("second_note_words_color")
      << chords_model_pointer->get_index(1, 0, words_column) << true;
  QTest::newRow("second_note_instrument_color")
      << chords_model_pointer->get_index(1, 0, instrument_column) << true;
}

void Tester::test_get_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);
  QCOMPARE(chords_model_pointer->data(index, role), value);
}

void Tester::test_get_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");
  QTest::newRow("first_chord_symbol")
      << chords_model_pointer->get_index(0) << Qt::DisplayRole << QVariant("♫");
  QTest::newRow("first_chord_decoration")
      << chords_model_pointer->get_index(0) << Qt::DecorationRole << QVariant();
  QTest::newRow("first_note_symbol") << chords_model_pointer->get_index(0, 0)
                                     << Qt::DisplayRole << QVariant("♪");
  QTest::newRow("first_note_decoration")
      << chords_model_pointer->get_index(0, 0) << Qt::DecorationRole
      << QVariant();
  QTest::newRow("second_note_interval")
      << chords_model_pointer->get_index(1, 0, interval_column)
      << Qt::DisplayRole << QVariant("2/2o1");
}

void Tester::test_delegate_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  auto *cell_editor_pointer = chords_view_pointer->create_editor(index);

  QCOMPARE(cell_editor_pointer->property(
               cell_editor_pointer->metaObject()->userProperty().name()),
           old_value);

  chords_view_pointer->set_editor(cell_editor_pointer, index, new_value);

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
}

void Tester::test_delegate_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("instrument editor")
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval editor")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats editor")
      << chords_model_pointer->get_index(0, -1, beats_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("volume editor")
      << chords_model_pointer->get_index(0, -1, volume_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("tempo editor")
      << chords_model_pointer->get_index(0, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("words editor")
      << chords_model_pointer->get_index(0, -1, words_column) << QVariant("")
      << QVariant("hello");
}

void Tester::test_set_value() const {
  // setData only works for the edit role
  QVERIFY(!(chords_model_pointer->setData(chords_model_pointer->get_index(0),
                                          QVariant(), Qt::DecorationRole)));

  // test undo merging
  auto new_interval = QVariant::fromValue(Interval(3, 2));
  auto first_chord_index =
      chords_model_pointer->get_index(0, -1, interval_column);
  QVERIFY(chords_model_pointer->setData(
      first_chord_index, QVariant::fromValue(Interval(5, 4)), Qt::EditRole));
  QVERIFY(chords_model_pointer->setData(first_chord_index, new_interval,
                                        Qt::EditRole));
  QCOMPARE(chords_model_pointer->data(first_chord_index, Qt::EditRole),
           new_interval);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->data(first_chord_index, Qt::EditRole),
           QVariant::fromValue(Interval()));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, old_display_value);
  QFETCH(const QVariant, new_value);
  QFETCH(const QVariant, new_display_value);

  QVERIFY(chords_model_pointer->setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole),
           new_display_value);

  undo_stack_pointer->undo();
  undo_stack_pointer->redo();
  undo_stack_pointer->undo();

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole),
           old_display_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("old_display_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_display_value");

  QTest::newRow("first_chord_interval")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_chord_beats")
      << chords_model_pointer->get_index(0, -1, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_volume")
      << chords_model_pointer->get_index(0, -1, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_tempo")
      << chords_model_pointer->get_index(0, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_words")
      << chords_model_pointer->get_index(0, -1, words_column) << QVariant("")
      << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe"))
      << QVariant("Oboe");
  QTest::newRow("first_note_interval")
      << chords_model_pointer->get_index(0, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_note_beats")
      << chords_model_pointer->get_index(0, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_volume")
      << chords_model_pointer->get_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_tempo")
      << chords_model_pointer->get_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_words")
      << chords_model_pointer->get_index(0, 0, words_column) << QVariant("")
      << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << chords_model_pointer->get_index(0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe"))
      << QVariant("Oboe");
}

void Tester::test_paste_cell_template() {
  QFETCH(const QModelIndex, old_index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QModelIndex, new_index);
  QFETCH(const QVariant, new_value);

  trigger_action(new_index, SELECT_CELL, copy_action_pointer);

  trigger_action(old_index, SELECT_CELL, paste_cell_action_pointer);

  QCOMPARE(chords_model_pointer->data(old_index, Qt::EditRole), new_value);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->data(old_index, Qt::EditRole), old_value);
}

void Tester::test_paste_cell_template_data() {
  QTest::addColumn<QModelIndex>("old_index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QModelIndex>("new_index");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("chord interval")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << QVariant::fromValue(Interval())
      << chords_model_pointer->get_index(1, -1, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("chord beats")
      << chords_model_pointer->get_index(0, -1, beats_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, -1, beats_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("chord tempo ratio")
      << chords_model_pointer->get_index(0, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("chord volume ratio")
      << chords_model_pointer->get_index(0, -1, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, -1, volume_ratio_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("chord words")
      << chords_model_pointer->get_index(0, -1, words_column) << QVariant("")
      << chords_model_pointer->get_index(1, -1, words_column)
      << QVariant("hello");

  QTest::newRow("chord instrument")
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << chords_model_pointer->get_index(1, -1, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));

  QTest::newRow("note interval")
      << chords_model_pointer->get_index(0, 0, interval_column)
      << QVariant::fromValue(Interval())
      << chords_model_pointer->get_index(1, 0, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("note beats")
      << chords_model_pointer->get_index(0, 0, beats_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, 0, beats_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("note tempo ratio")
      << chords_model_pointer->get_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("note volume ratio")
      << chords_model_pointer->get_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_index(1, 0, volume_ratio_column)
      << QVariant::fromValue(Rational(2));

  QTest::newRow("note words")
      << chords_model_pointer->get_index(0, 0, words_column) << QVariant("")
      << chords_model_pointer->get_index(1, 0, words_column)
      << QVariant("hello");

  QTest::newRow("note instrument")
      << chords_model_pointer->get_index(0, 0, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << chords_model_pointer->get_index(1, 0, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));
}

void Tester::test_paste_wrong_cell_template() {
  QFETCH(const QModelIndex, old_index);
  QFETCH(const QModelIndex, new_index);
  QFETCH(const QString, error_message);

  trigger_action(old_index, SELECT_CELL, copy_action_pointer);

  close_message_later(error_message.toStdString());
  trigger_action(new_index, SELECT_CELL, paste_cell_action_pointer);
}

void Tester::test_paste_wrong_cell_template_data() {
  QTest::addColumn<QModelIndex>("old_index");
  QTest::addColumn<QModelIndex>("new_index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("interval to rational")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << chords_model_pointer->get_index(0, -1, beats_column)
      << "Cannot paste an interval into Beats column";

  QTest::newRow("interval to words")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << chords_model_pointer->get_index(0, -1, words_column)
      << "Cannot paste an interval into Words column";

  QTest::newRow("interval to instrument")
      << chords_model_pointer->get_index(0, -1, interval_column)
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << "Cannot paste an interval into Instrument column";

  QTest::newRow("rational to interval")
      << chords_model_pointer->get_index(0, -1, beats_column)
      << chords_model_pointer->get_index(0, -1, interval_column)
      << "Cannot paste a rational into Interval column";
}

void Tester::test_insert_delete() const {
  trigger_action(chords_model_pointer->get_index(2), SELECT_ROWS,
                 insert_into_action_pointer);
  QCOMPARE(chords_model_pointer->rowCount(chords_model_pointer->get_index(2)),
           1);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(chords_model_pointer->get_index(2)),
           0);

  // test chord templating from previous chord
  trigger_action(chords_model_pointer->get_index(1), SELECT_ROWS,
                 insert_after_action_pointer);
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_index(1, -1, beats_column), Qt::EditRole),
      QVariant::fromValue(Rational(2)));
  undo_stack_pointer->undo();

  // test note templating from previous note
  trigger_action(chords_model_pointer->get_index(1, 0), SELECT_ROWS,
                 insert_after_action_pointer);
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_index(2, 0, beats_column), Qt::EditRole),
      QVariant::fromValue(Rational(2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_index(2, 0, volume_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_index(2, 0, tempo_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_index(2, 0, words_column), Qt::EditRole),
      "hello");
  undo_stack_pointer->undo();

  // test note inheritance from chord
  trigger_action(chords_model_pointer->get_index(0, 1), SELECT_ROWS,
                 insert_before_action_pointer);
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_index(0, 1, beats_column), Qt::EditRole),
      QVariant::fromValue(Rational(2)));
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_index(0, 1, words_column), Qt::EditRole),
      "hello");
  undo_stack_pointer->undo();
}

void Tester::test_insert_delete_sibling_template() {
  QFETCH(int, parent_number);
  QFETCH(int, child_number);
  QFETCH(QAction *, action_pointer);
  QFETCH(int, old_row_count);
  QFETCH(int, new_row_count);

  trigger_action(chords_model_pointer->get_index(child_number, parent_number),
                 SELECT_ROWS, action_pointer);
  auto parent_index = chords_model_pointer->parent(
      chords_model_pointer->get_index(child_number, parent_number));
  QCOMPARE(chords_model_pointer->rowCount(parent_index), new_row_count);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_insert_delete_sibling_template_data() {
  QTest::addColumn<int>("parent_number");
  QTest::addColumn<int>("child_number");
  QTest::addColumn<QAction *>("action_pointer");
  QTest::addColumn<int>("old_row_count");
  QTest::addColumn<int>("new_row_count");

  QTest::newRow("insert chord before")
      << -1 << 0 << insert_before_action_pointer << 3 << 4;
  QTest::newRow("insert chord after")
      << -1 << 0 << insert_after_action_pointer << 3 << 4;
  QTest::newRow("delete chord") << -1 << 0 << remove_action_pointer << 3 << 2;

  QTest::newRow("insert note before")
      << 0 << 0 << insert_before_action_pointer << 2 << 3;
  QTest::newRow("insert note after")
      << 0 << 0 << insert_after_action_pointer << 2 << 3;
  QTest::newRow("delete note") << 0 << 0 << remove_action_pointer << 2 << 1;
}

void Tester::test_paste_siblings_template() {
  QFETCH(QAction *, action_pointer);
  QFETCH(int, parent_number);
  QFETCH(int, child_number);
  QFETCH(int, parent_row_count);

  trigger_action(chords_model_pointer->get_index(child_number, parent_number),
                 SELECT_ROWS, copy_action_pointer);
  trigger_action(chords_model_pointer->get_index(child_number, parent_number),
                 SELECT_ROWS, action_pointer);
  auto parent_index = chords_model_pointer->parent(
      chords_model_pointer->get_index(child_number, parent_number));
  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count + 1);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count);
}

void Tester::test_paste_siblings_template_data() {
  QTest::addColumn<QAction *>("action_pointer");
  QTest::addColumn<int>("parent_number");
  QTest::addColumn<int>("child_number");
  QTest::addColumn<int>("parent_row_count");

  QTest::newRow("paste chord before")
      << paste_before_action_pointer << -1 << 0 << 3;

  QTest::newRow("paste chord after")
      << paste_after_action_pointer << -1 << 0 << 3;

  QTest::newRow("paste note before")
      << paste_before_action_pointer << 0 << 0 << 2;

  QTest::newRow("paste note after")
      << paste_after_action_pointer << 0 << 0 << 2;
}

void Tester::test_bad_paste_template() {
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QModelIndex, index);
  QFETCH(const QItemSelectionModel::SelectionFlags, flags);
  QFETCH(QAction *, action_pointer);
  QFETCH(const QString, error_message);

  close_message_later(error_message.toStdString());
  copy_text(copied.toStdString(), mime_type.toStdString());
  trigger_action(index, flags, action_pointer);
}

void Tester::test_bad_paste_template_data() {
  QTest::addColumn<QString>("copied");
  QTest::addColumn<QString>("mime_type");
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QItemSelectionModel::SelectionFlags>("flags");
  QTest::addColumn<QAction *>("action_pointer");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("unparsable chord")
      << "[" << CHORDS_MIME << chords_model_pointer->get_index(0) << SELECT_ROWS
      << paste_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong type chord")
      << "{}" << CHORDS_MIME << chords_model_pointer->get_index(0)
      << SELECT_ROWS << paste_after_action_pointer
      << "At  of {} - unexpected instance type\n";

  QTest::newRow("unparsable note")
      << "[" << NOTES_MIME << chords_model_pointer->get_index(0, 0)
      << SELECT_ROWS << paste_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong type note")
      << "{}" << NOTES_MIME << chords_model_pointer->get_index(0, 0)
      << SELECT_ROWS << paste_after_action_pointer
      << "At  of {} - unexpected instance type\n";

  QTest::newRow("wrong row mime type")
      << "{}"
      << "not a mime" << chords_model_pointer->get_index(0) << SELECT_ROWS
      << paste_after_action_pointer << "Cannot paste MIME type \"not a mime\"";

  QTest::newRow("wrong cell mime type")
      << "{}"
      << "not a mime" << chords_model_pointer->get_index(0, -1, interval_column)
      << SELECT_CELL << paste_cell_action_pointer
      << "Cannot paste MIME type \"not a mime\"";

  QTest::newRow("unparsable interval")
      << "[" << INTERVAL_MIME
      << chords_model_pointer->get_index(0, -1, interval_column) << SELECT_CELL
      << paste_cell_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable rational")
      << "[" << RATIONAL_MIME
      << chords_model_pointer->get_index(0, -1, beats_column) << SELECT_CELL
      << paste_cell_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable instrument")
      << "[" << INSTRUMENT_MIME
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << SELECT_CELL << paste_cell_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable octave")
      << "[" << WORDS_MIME
      << chords_model_pointer->get_index(0, -1, words_column) << SELECT_CELL
      << paste_cell_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong interval type")
      << "[]" << INTERVAL_MIME
      << chords_model_pointer->get_index(0, -1, interval_column) << SELECT_CELL
      << paste_cell_action_pointer << "At  of [] - unexpected instance type\n";

  QTest::newRow("wrong rational type")
      << "[]" << RATIONAL_MIME
      << chords_model_pointer->get_index(0, -1, beats_column) << SELECT_CELL
      << paste_cell_action_pointer << "At  of [] - unexpected instance type\n";

  QTest::newRow("wrong instrument type")
      << "[]" << INSTRUMENT_MIME
      << chords_model_pointer->get_index(0, -1, instrument_column)
      << SELECT_CELL << paste_cell_action_pointer
      << "At  of [] - unexpected instance type\n";

  QTest::newRow("wrong words type")
      << "[]" << WORDS_MIME
      << chords_model_pointer->get_index(0, -1, words_column) << SELECT_CELL
      << paste_cell_action_pointer << "At  of [] - unexpected instance type\n";
}

void Tester::test_paste_rows() {
  // copy chord
  trigger_action(chords_model_pointer->get_index(0), SELECT_ROWS,
                 copy_action_pointer);

  // can't paste chord as a note
  close_message_later("Cannot paste chords into another chord!");
  trigger_action(chords_model_pointer->get_index(0, 0), SELECT_ROWS,
                 paste_after_action_pointer);

  // copy note
  trigger_action(chords_model_pointer->get_index(0, 0), SELECT_ROWS,
                 copy_action_pointer);

  // paste note into
  trigger_action(chords_model_pointer->get_index(2), SELECT_ROWS,
                 paste_into_action_pointer);
  QCOMPARE(chords_model_pointer->rowCount(chords_model_pointer->get_index(2)),
           1);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(chords_model_pointer->get_index(2)),
           0);

  // can't paste note as chord
  close_message_later("Can only paste notes into a chord!");
  trigger_action(chords_model_pointer->get_index(0), SELECT_ROWS,
                 paste_after_action_pointer);
}

void Tester::test_play() {
  // Test volume errors
  QVERIFY(chords_model_pointer->setData(
      chords_model_pointer->get_index(0, 0, volume_ratio_column),
      QVariant::fromValue(Rational(10)), Qt::EditRole));

  close_message_later(
      "Volume exceeds 100% for chord 1, note 1. Playing with 100% volume.");
  trigger_action(chords_model_pointer->get_index(0, 0), SELECT_ROWS,
                 play_action_pointer);
  QThread::msleep(WAIT_TIME);
  stop_playing_action_pointer->trigger();
  undo_stack_pointer->undo();

  // Test midi overload
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    trigger_action(chords_model_pointer->get_index(0, 0), SELECT_ROWS,
                   insert_before_action_pointer);
  }

  close_message_later(
      "Out of MIDI channels for chord 1, note 17. Not playing note.");
  trigger_action(chords_model_pointer->get_index(0), SELECT_ROWS,
                 play_action_pointer);
  QThread::msleep(WAIT_TIME);
  stop_playing_action_pointer->trigger();

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    undo_stack_pointer->undo();
  }
}

void Tester::test_play_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  selector_pointer->select(QItemSelection(first_index, last_index),
                           SELECT_ROWS);
  play_action_pointer->trigger();
  // first cut off early
  play_action_pointer->trigger();
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  clear_selection();
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords") << chords_model_pointer->get_index(0)
                                    << chords_model_pointer->get_index(1);
  QTest::newRow("second chord") << chords_model_pointer->get_index(1)
                                << chords_model_pointer->get_index(1);

  auto first_chord_second_note_index = chords_model_pointer->get_index(1, 0);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index = chords_model_pointer->get_index(0, 1);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_io() {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  auto std_file_name = temp_json_file.fileName().toStdString();
  song_editor.save_as_file(std_file_name);
  QCOMPARE(song_editor.current_file, std_file_name);
  save_action_pointer->trigger();

  song_editor.export_to_file(std_file_name);

  QTemporaryFile broken_json_file;
  broken_json_file.open();
  broken_json_file.write("{");
  broken_json_file.close();
  close_message_later(
      "[json.exception.parse_error.101] parse error at line 1, column 2: "
      "syntax error while parsing object key - unexpected end of input; "
      "expected string literal");
  song_editor.open_file(broken_json_file.fileName().toStdString());

  QTemporaryFile wrong_type_json_file;
  wrong_type_json_file.open();
  wrong_type_json_file.write("[]");
  wrong_type_json_file.close();
  close_message_later("At  of [] - unexpected instance type\n");
  song_editor.open_file(wrong_type_json_file.fileName().toStdString());
}