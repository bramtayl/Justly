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

void close_message() {
  for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
    auto *box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
    if (box_pointer != nullptr) {
      QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
      return;
    }
  }
}

void Tester::initTestCase() {
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &close_message);
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

  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(1),
           1);
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

void Tester::test_playback_volume_control() {
  song_editor.set_playback_volume(1);
  QCOMPARE(song_editor.get_playback_volume(), 1);
}

void Tester::test_starting_instrument_control() const {
  const auto *original_value = get_instrument_pointer("Marimba");
  const auto *new_value = get_instrument_pointer("Oboe");
  const auto *new_value_2 = get_instrument_pointer("Ocarina");

  const auto *old_value =
      song_editor.starting_instrument_pointer;
  QCOMPARE(old_value, get_instrument_pointer("Marimba"));

  // test change
  song_editor.starting_instrument_editor_pointer->setValue(new_value);
  QCOMPARE(song_editor.starting_instrument_pointer, new_value);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer, old_value);

  // test redo
  song_editor.undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_instrument_pointer, new_value);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer,
           original_value);

  // test combining
  song_editor.starting_instrument_editor_pointer->setValue(new_value);
  song_editor.starting_instrument_editor_pointer->setValue(new_value_2);
  QCOMPARE(song_editor.starting_instrument_pointer,
           new_value_2);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_instrument_pointer,
           original_value);
}

void Tester::test_starting_key_control() const {
  auto old_value = song_editor.starting_key;
  QCOMPARE(old_value, ORIGINAL_KEY);

  // test change
  song_editor.starting_key_editor_pointer->setValue(STARTING_KEY_1);
  QCOMPARE(song_editor.starting_key, STARTING_KEY_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, old_value);

  // test redo
  song_editor.undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_key, STARTING_KEY_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, ORIGINAL_KEY);

  // test combining
  song_editor.starting_key_editor_pointer->setValue(STARTING_KEY_1);
  song_editor.starting_key_editor_pointer->setValue(STARTING_KEY_2);
  QCOMPARE(song_editor.starting_key, STARTING_KEY_2);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_key, ORIGINAL_KEY);
}

void Tester::test_starting_volume_control() const {
  auto old_value = song_editor.starting_volume_percent;
  QCOMPARE(old_value, ORIGINAL_VOLUME);

  // test change
  song_editor.starting_volume_editor_pointer->setValue(STARTING_VOLUME_1);
  QCOMPARE(song_editor.starting_volume_percent,
           STARTING_VOLUME_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, old_value);

  // test redo
  song_editor.undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_volume_percent,
           STARTING_VOLUME_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent,
           ORIGINAL_VOLUME);

  // test combining
  song_editor.starting_volume_editor_pointer->setValue(STARTING_VOLUME_1);
  song_editor.starting_volume_editor_pointer->setValue(STARTING_VOLUME_2);
  QCOMPARE(song_editor.starting_volume_percent,
           STARTING_VOLUME_2);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent,
           ORIGINAL_VOLUME);
}

void Tester::test_starting_tempo_control() const {
  auto old_value = song_editor.starting_tempo;
  QCOMPARE(old_value, ORIGINAL_TEMPO);

  // test change
  song_editor.starting_tempo_editor_pointer->setValue(STARTING_TEMPO_1);
  QCOMPARE(song_editor.starting_tempo,
           STARTING_TEMPO_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, old_value);

  // test redo
  song_editor.undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_tempo,
           STARTING_TEMPO_1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, ORIGINAL_TEMPO);

  // test combining
  song_editor.starting_tempo_editor_pointer->setValue(STARTING_TEMPO_1);
  song_editor.starting_tempo_editor_pointer->setValue(STARTING_TEMPO_2);
  QCOMPARE(song_editor.starting_tempo,
           STARTING_TEMPO_2);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_tempo, ORIGINAL_TEMPO);
}

void Tester::test_tree() const {
  // test song
  QCOMPARE(song_editor.chords_view_pointer->model()->rowCount(
               QModelIndex()),
           3);
  QCOMPARE(song_editor.chords_view_pointer->model()->columnCount(
               QModelIndex()),
           NOTE_CHORD_COLUMNS);

  QCOMPARE(song_editor.chords_view_pointer->model()->parent(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   -1, 0)),
           QModelIndex());
  // only nest the symbol column
  QCOMPARE(song_editor.chords_view_pointer->model()->rowCount(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   -1, 0, interval_column)),
           0);

  // test first note
  QCOMPARE(song_editor.chords_view_pointer->model()
               ->parent(song_editor.chords_view_pointer->chords_model_pointer
                            ->get_index(0, 0))
               .row(),
           0);
}

void Tester::test_copy_paste() const {
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.copy_action_pointer->trigger();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.paste_before_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           4);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.paste_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           4);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.copy_action_pointer->trigger();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.paste_before_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           3);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.paste_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           3);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 2),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.paste_into_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(2),
           1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(2),
           0);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->chords_model_pointer->paste_rows_text(
      0, "[", QModelIndex());

  song_editor.chords_view_pointer->chords_model_pointer->paste_rows_text(
      0, "{}", QModelIndex());

  song_editor.chords_view_pointer->chords_model_pointer->paste_rows_text(
      0, "[",
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0));

  song_editor.chords_view_pointer->chords_model_pointer->paste_rows_text(
      0, "{}",
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0));
}

void Tester::test_insert_delete() const {
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 2),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_into_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(2),
           1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(2),
           0);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_before_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           3);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           3);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.remove_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           1);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(0),
           2);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_before_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           4);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.remove_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           2);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           4);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer
               ->get_number_of_children(-1),
           3);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  // test chord templating from previous chord
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 1),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   -1, 1, beats_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  song_editor.undo_stack_pointer->undo();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  // test note templating from previous note
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 1),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_after_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   0, 2, beats_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   0, 2, volume_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   0, 2, tempo_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   0, 2, words_column),
               Qt::EditRole),
           "hello");
  song_editor.undo_stack_pointer->undo();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  // test note inheritance from chord
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 1),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.insert_into_action_pointer->trigger();
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   1, 0, beats_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               song_editor.chords_view_pointer->chords_model_pointer->get_index(
                   1, 0, words_column),
               Qt::EditRole),
           "hello");
  song_editor.undo_stack_pointer->undo();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);
}

void Tester::test_column_headers_template() const {
  QFETCH(const NoteChordField, field);
  QFETCH(const QVariant, value);

  QCOMPARE(song_editor.chords_view_pointer->chords_model_pointer->headerData(
               field, Qt::Horizontal, Qt::DisplayRole),
           value);
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
  song_editor.chords_view_pointer->selectionModel()->select(
      first_index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.chords_view_pointer->selectionModel()->select(
      second_index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  auto selected_rows =
      song_editor.chords_view_pointer->selectionModel()->selectedRows();
  QCOMPARE(selected_rows.size(), 1);
  QCOMPARE(selected_rows[0], first_index);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);
}

void Tester::test_select_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("second_index");

  QTest::newRow("select_chord_then_note")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0);
  QTest::newRow("select_note_then_chord")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0)
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1,
                                                                          0);
}

void Tester::test_flags_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemFlags, item_flags);
  QCOMPARE(song_editor.chords_view_pointer->model()->flags(index),
           item_flags);
}

void Tester::test_flags_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemFlags>("item_flags");

  QTest::newRow("first_chord_symbol_flag")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first_chord_interval_flag")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, interval_column)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

void Tester::test_colors_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const bool, non_default);
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::ForegroundRole),
           non_default ? NON_DEFAULT_COLOR : DEFAULT_COLOR);
}

void Tester::test_colors_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << true;
  QTest::newRow("first_chord_interval_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, interval_column)
      << false;
  QTest::newRow("first_chord_beats_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, beats_column)
      << false;
  QTest::newRow("first_chord_volume_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, volume_ratio_column)
      << false;
  QTest::newRow("first_chord_tempo_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, tempo_ratio_column)
      << false;
  QTest::newRow("first_chord_words_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, words_column)
      << false;
  QTest::newRow("first_chord_instrument_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, instrument_column)
      << false;

  QTest::newRow("second_chord_interval_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 1, interval_column)
      << true;
  QTest::newRow("second_chord_beats_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 1, beats_column)
      << true;
  QTest::newRow("second_chord_volume_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 1, volume_ratio_column)
      << true;
  QTest::newRow("second_chord_tempo_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 1, tempo_ratio_column)
      << true;
  QTest::newRow("second_chord_words_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 1, words_column)
      << true;

  QTest::newRow("first_note_symbol_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0)
      << true;
  QTest::newRow("first_note_interval_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, interval_column)
      << false;
  QTest::newRow("first_note_beats_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, beats_column)
      << false;
  QTest::newRow("first_note_volume_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, volume_ratio_column)
      << false;
  QTest::newRow("first_note_tempo_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, tempo_ratio_column)
      << false;
  QTest::newRow("first_note_words_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, words_column)
      << false;
  QTest::newRow("first_note_instrument_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, instrument_column)
      << false;

  QTest::newRow("second_note_interval_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, interval_column)
      << true;
  QTest::newRow("second_note_beats_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, beats_column)
      << true;
  QTest::newRow("second_note_volume_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, volume_ratio_column)
      << true;
  QTest::newRow("second_note_tempo_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, tempo_ratio_column)
      << true;
  QTest::newRow("second_note_words_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, words_column)
      << true;
  QTest::newRow("second_note_instrument_color")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, instrument_column)
      << true;
}

void Tester::test_get_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);
  QCOMPARE(
      song_editor.chords_view_pointer->model()->data(index, role),
      value);
}

void Tester::test_get_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");
  QTest::newRow("first_chord_symbol")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << Qt::DisplayRole << QVariant("♫");
  QTest::newRow("first_chord_decoration")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << Qt::DecorationRole << QVariant();
  QTest::newRow("first_note_symbol")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0)
      << Qt::DisplayRole << QVariant("♪");
  QTest::newRow("first_note_decoration")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0)
      << Qt::DecorationRole << QVariant();
  QTest::newRow("second_note_interval")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 1, interval_column)
      << Qt::DisplayRole << QVariant("2/2o1");
}

void Tester::test_delegate_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  auto *cell_editor_pointer =
      song_editor.chords_view_pointer->create_editor(index);

  QCOMPARE(cell_editor_pointer->property(
               cell_editor_pointer->metaObject()->userProperty().name()),
           old_value);

  song_editor.chords_view_pointer->set_editor(cell_editor_pointer, index,
                                              new_value);

  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::EditRole),
           new_value);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::EditRole),
           old_value);
}

void Tester::test_delegate_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("instrument editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, interval_column)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, beats_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("volume editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, volume_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("tempo editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("words editor")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, words_column)
      << QVariant("") << QVariant("hello");
}

void Tester::test_set_value() const {
  QVERIFY(song_editor.chords_view_pointer->model()->setData(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QVariant(), Qt::EditRole));
  // setData only works for the edit role
  QVERIFY(!(song_editor.chords_view_pointer->model()->setData(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QVariant(), Qt::DecorationRole)));

  // test undo merging
  auto new_interval = QVariant::fromValue(Interval(3, 2));
  auto first_chord_index =
      song_editor.chords_view_pointer->chords_model_pointer->get_index(
          -1, 0, interval_column);
  QVERIFY(song_editor.chords_view_pointer->model()->setData(
      first_chord_index, QVariant::fromValue(Interval(5, 4)), Qt::EditRole));
  QVERIFY(song_editor.chords_view_pointer->model()->setData(
      first_chord_index, new_interval, Qt::EditRole));
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               first_chord_index, Qt::EditRole),
           new_interval);
  song_editor.undo_stack_pointer->undo();
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               first_chord_index, Qt::EditRole),
           QVariant::fromValue(Interval()));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, old_display_value);
  QFETCH(const QVariant, new_value);
  QFETCH(const QVariant, new_display_value);

  QVERIFY(song_editor.chords_view_pointer->model()->setData(
      index, new_value, Qt::EditRole));

  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::EditRole),
           new_value);
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::DisplayRole),
           new_display_value);

  song_editor.undo_stack_pointer->undo();
  song_editor.undo_stack_pointer->redo();
  song_editor.undo_stack_pointer->undo();

  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::EditRole),
           old_value);
  QCOMPARE(song_editor.chords_view_pointer->model()->data(
               index, Qt::DisplayRole),
           old_display_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("old_display_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_display_value");

  QTest::newRow("first_chord_interval")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_chord_beats")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_volume")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_tempo")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_words")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, words_column)
      << QVariant("") << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             -1, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe"))
      << QVariant("Oboe");
  QTest::newRow("first_note_interval")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_note_beats")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_volume")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_tempo")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_words")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, words_column)
      << QVariant("") << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(
             0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe"))
      << QVariant("Oboe");
}

void Tester::test_io() {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  auto std_file_name = temp_json_file.fileName().toStdString();
  song_editor.save_as_file(std_file_name);
  QCOMPARE(song_editor.current_file, std_file_name);
  song_editor.save_action_pointer->trigger();

  const QTemporaryFile temp_wav_file;
  temp_json_file.open();
  temp_json_file.close();
  song_editor.export_to_file(std_file_name);

  const QTemporaryFile broken_json_file;
  temp_json_file.open();
  temp_json_file.write("{");
  temp_json_file.close();
  song_editor.open_file(broken_json_file.fileName().toStdString());
}

void Tester::test_play() {
  // Test volume errors
  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(
          0, 0, volume_ratio_column),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  QVERIFY(song_editor.chords_view_pointer->model()->setData(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(
          0, 0, volume_ratio_column),
      QVariant::fromValue(Rational(10)), Qt::EditRole));
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &close_message);
  timer_pointer->start(WAIT_TIME);
  song_editor.play_action_pointer->trigger();
  song_editor.stop_playing_action_pointer->trigger();
  song_editor.undo_stack_pointer->undo();

  // Test midi overload
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    song_editor.chords_view_pointer->selectionModel()->select(
        song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 0),
        QItemSelectionModel::Select | QItemSelectionModel::Rows);
    song_editor.insert_before_action_pointer->trigger();
    song_editor.chords_view_pointer->selectionModel()->select(
        QModelIndex(), QItemSelectionModel::Clear);
  }

  QTimer *const timer_pointer_2 = std::make_unique<QTimer>(this).release();
  connect(timer_pointer_2, &QTimer::timeout, this, &close_message);
  timer_pointer_2->start(WAIT_TIME);

  song_editor.chords_view_pointer->selectionModel()->select(
      song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  song_editor.play_action_pointer->trigger();
  song_editor.stop_playing_action_pointer->trigger();
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    song_editor.undo_stack_pointer->undo();
  }
}

void Tester::test_play_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  song_editor.chords_view_pointer->selectionModel()->select(
      QItemSelection(first_index, last_index),
      QItemSelectionModel::Select | QItemSelectionModel::Rows);
  // use the second chord to test key changing
  song_editor.play_action_pointer->trigger();
  // first cut off early
  song_editor.play_action_pointer->trigger();
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  song_editor.chords_view_pointer->selectionModel()->select(
      QModelIndex(), QItemSelectionModel::Clear);
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 0)
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1,
                                                                          1);
  QTest::newRow("second chord")
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1, 1)
      << song_editor.chords_view_pointer->chords_model_pointer->get_index(-1,
                                                                          1);

  auto first_chord_second_note_index =
      song_editor.chords_view_pointer->chords_model_pointer->get_index(0, 1);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index =
      song_editor.chords_view_pointer->chords_model_pointer->get_index(1, 0);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}
