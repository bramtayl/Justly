#include "tests/Tester.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QMetaObject>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTemporaryFile>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <memory>
#include <string>

#include "justly/ChordsModel.hpp"
#include "justly/ChordsView.hpp"
#include "justly/Instrument.hpp"
#include "justly/InstrumentEditor.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "justly/Rational.hpp"
#include "justly/SongEditor.hpp"

const auto ORIGINAL_KEY = 220.0;
const auto STARTING_KEY_1 = 401.0;
const auto STARTING_KEY_2 = 402.0;

const auto ORIGINAL_TEMPO = 200.0;
const auto STARTING_TEMPO_1 = 150.0;
const auto STARTING_TEMPO_2 = 100.0;

const auto ORIGINAL_VOLUME = 50.0;
const auto STARTING_VOLUME_1 = 51.0;
const auto STARTING_VOLUME_2 = 52.0;

const auto WAIT_TIME = 500;

const auto OVERLOAD_NUMBER = 15;

const auto NEW_VOLUME_PERCENT = 100;

const auto PERCENT = 100;
const auto MAX_GAIN = 10;

const auto SELECT_ROWS =
    QItemSelectionModel::Select | QItemSelectionModel::Rows;

const auto SELECT_CELL = QFlags(QItemSelectionModel::Select);

const auto *const SONG_TEXT = R""""({
    "chords": [
        {
            "notes": [
                {},
                {
                    "beats": {
                        "denominator": 2,
                        "numerator": 2
                    },
                    "instrument": "Oboe",
                    "interval": {
                        "denominator": 2,
                        "numerator": 2,
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "denominator": 2,
                        "numerator": 2
                    },
                    "volume_ratio": {
                        "denominator": 2,
                        "numerator": 2
                    },
                    "words": "hello"
                }
            ]
        },
        {
            "beats": {
                "denominator": 2,
                "numerator": 2
            },
            "instrument": "Oboe",
            "interval": {
                "denominator": 2,
                "numerator": 2,
                "octave": 1
            },
            "notes": [
                {}
            ],
            "tempo_ratio": {
                "denominator": 2,
                "numerator": 2
            },
            "volume_ratio": {
                "denominator": 2,
                "numerator": 2
            },
            "words": "hello"
        },
        {}
    ],
    "starting_instrument": "Marimba",
    "starting_key": 220.0,
    "starting_tempo": 200.0,
    "starting_volume_percent": 50.0
})"""";

void Tester::close_message_later(const QString &expected_text) {
  auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  QTimer *timer_pointer = std::make_unique<QTimer>(this).release();
  timer_pointer->setSingleShot(true);
  connect(timer_pointer, &QTimer::timeout, this, [this, expected_text]() {
    for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
      auto *box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
      if (box_pointer != nullptr) {
        auto actual_text = box_pointer->text();
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
  register_converters();
  QTemporaryFile main_file;
  if (main_file.open()) {
    main_file.write(SONG_TEXT);
    main_file.close();
  }
  song_editor.open_file(main_file.fileName());
}

void Tester::test_to_string_template() {
  QFETCH(const QVariant, value);
  QFETCH(const QString, text);

  QCOMPARE(value.toString(), text);
}

void Tester::test_to_string_template_data() {
  QTest::addColumn<QVariant>("value");
  QTest::addColumn<QString>("text");

  QTest::newRow("denominator interval")
      << QVariant::fromValue(Interval(1, 2)) << "/2";
  QTest::newRow("numerator octave interval")
      << QVariant::fromValue(Interval(2, 1, 1)) << "2o1";
  QTest::newRow("denominator rational")
      << QVariant::fromValue(Rational(1, 2)) << "/2";
  QTest::newRow("numerator denominator rational")
      << QVariant::fromValue(Rational(2, 2)) << "2/2";
}

void Tester::test_row_count_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const int, row_count);

  QCOMPARE(chords_model_pointer->rowCount(index), row_count);
}

void Tester::test_row_count_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("row_count");

  QTest::newRow("song") << QModelIndex() << 3;
  QTest::newRow("first chord") << chords_model_pointer->get_chord_index(0) << 2;
  QTest::newRow("second chord")
      << chords_model_pointer->get_chord_index(1) << 1;
  QTest::newRow("non-symbol chord")
      << chords_model_pointer->get_chord_index(0, interval_column) << 0;
}

void Tester::test_parent_template() {
  QFETCH(const QModelIndex, child_index);
  QFETCH(const QModelIndex, parent_index);

  QCOMPARE(chords_model_pointer->parent(child_index), parent_index);
}

void Tester::test_parent_template_data() {
  QTest::addColumn<QModelIndex>("child_index");
  QTest::addColumn<QModelIndex>("parent_index");

  QTest::newRow("chord parent")
      << chords_model_pointer->get_chord_index(0) << QModelIndex();
  QTest::newRow("note parent") << chords_model_pointer->get_note_index(0, 0)
                               << chords_model_pointer->get_chord_index(0);
}

void Tester::test_column_count() const {
  QCOMPARE(chords_model_pointer->columnCount(QModelIndex()),
           NUMBER_OF_NOTE_CHORD_COLUMNS);
}

void Tester::test_playback_volume_control() {
  auto old_playback_volume = song_editor.get_playback_volume();
  song_editor.playback_volume_editor_pointer->setValue(NEW_VOLUME_PERCENT);
  QCOMPARE(song_editor.get_playback_volume(),
           NEW_VOLUME_PERCENT / PERCENT * MAX_GAIN);
  song_editor.playback_volume_editor_pointer->setValue(
      static_cast<int>(old_playback_volume * PERCENT / MAX_GAIN));
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
  starting_volume_percent_editor_pointer->setValue(STARTING_VOLUME_1);
  QCOMPARE(song_editor.starting_volume_percent, STARTING_VOLUME_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, old_value);

  // test redo
  undo_stack_pointer->redo();
  QCOMPARE(song_editor.starting_volume_percent, STARTING_VOLUME_1);
  undo_stack_pointer->undo();
  QCOMPARE(song_editor.starting_volume_percent, ORIGINAL_VOLUME);

  // test combining
  starting_volume_percent_editor_pointer->setValue(STARTING_VOLUME_1);
  starting_volume_percent_editor_pointer->setValue(STARTING_VOLUME_2);
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
  QFETCH(const NoteChordColumn, field);
  QFETCH(const QVariant, value);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);

  QCOMPARE(chords_model_pointer->headerData(field, orientation, role), value);
}

void Tester::test_column_headers_template_data() {
  QTest::addColumn<NoteChordColumn>("field");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("symbol header")
      << type_column << Qt::Horizontal << Qt::DisplayRole << QVariant("Type");
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
  QTest::newRow("wrong role")
      << type_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
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
      << chords_model_pointer->get_chord_index(0)
      << chords_model_pointer->get_note_index(0, 0);
  QTest::newRow("select_note_then_chord")
      << chords_model_pointer->get_note_index(0, 0)
      << chords_model_pointer->get_chord_index(0);
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
      << chords_model_pointer->get_chord_index(0)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first_chord_interval_flag")
      << chords_model_pointer->get_chord_index(0, interval_column)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
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
      << chords_model_pointer->get_chord_index(0) << Qt::DisplayRole
      << QVariant("♫");
  QTest::newRow("first_chord_decoration")
      << chords_model_pointer->get_chord_index(0) << Qt::DecorationRole
      << QVariant();
  QTest::newRow("first_note_symbol")
      << chords_model_pointer->get_note_index(0, 0) << Qt::DisplayRole
      << QVariant("♪");
  QTest::newRow("first_note_decoration")
      << chords_model_pointer->get_note_index(0, 0) << Qt::DecorationRole
      << QVariant();
  QTest::newRow("second_note_interval")
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << Qt::DisplayRole << QVariant::fromValue(Interval(2, 2, 1));
}

void Tester::get_different_values_template() const {
  QFETCH(const QModelIndex, index_1);
  QFETCH(const QModelIndex, index_2);
  QFETCH(const Qt::ItemDataRole, role);
  QCOMPARE_NE(chords_model_pointer->data(index_1, role),
              chords_model_pointer->data(index_2, role));
}

void Tester::get_different_values_template_data() const {
  QTest::addColumn<QModelIndex>("index_1");
  QTest::addColumn<QModelIndex>("index_2");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::newRow("chord/note background")
      << chords_model_pointer->get_chord_index(0)
      << chords_model_pointer->get_note_index(0, 0) << Qt::BackgroundRole;
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
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval editor")
      << chords_model_pointer->get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats editor")
      << chords_model_pointer->get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("volume editor")
      << chords_model_pointer->get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("tempo editor")
      << chords_model_pointer->get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("words editor")
      << chords_model_pointer->get_chord_index(0, words_column) << QVariant("")
      << QVariant("hello");
}

void Tester::test_set_value() const {
  // setData only works for the edit role
  QVERIFY(
      !(chords_model_pointer->setData(chords_model_pointer->get_chord_index(0),
                                      QVariant(), Qt::DecorationRole)));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  QVERIFY(chords_model_pointer->setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), new_value);

  undo_stack_pointer->undo();
  undo_stack_pointer->redo();
  undo_stack_pointer->undo();

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), old_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("first_chord_interval")
      << chords_model_pointer->get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant::fromValue(Interval(2));
  QTest::newRow("first_chord_beats")
      << chords_model_pointer->get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_volume")
      << chords_model_pointer->get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_tempo")
      << chords_model_pointer->get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_words")
      << chords_model_pointer->get_chord_index(0, words_column) << QVariant("")
      << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("first_note_interval")
      << chords_model_pointer->get_note_index(0, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant::fromValue(Interval(2));
  QTest::newRow("first_note_beats")
      << chords_model_pointer->get_note_index(0, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_volume")
      << chords_model_pointer->get_note_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_tempo")
      << chords_model_pointer->get_note_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_words")
      << chords_model_pointer->get_note_index(0, 0, words_column)
      << QVariant("") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << chords_model_pointer->get_note_index(0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
}

void Tester::test_delete_cell_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, empty_value);
  QFETCH(const QVariant, old_value);

  trigger_action(index, SELECT_CELL, delete_action_pointer);
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), empty_value);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
};

void Tester::test_delete_cell_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("empty_value");
  QTest::addColumn<QVariant>("old_value");

  QTest::newRow("second chord interval")
      << chords_model_pointer->get_chord_index(1, interval_column)
      << QVariant::fromValue(Interval())
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("second chord beats")
      << chords_model_pointer->get_chord_index(1, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("second chord words")
      << chords_model_pointer->get_chord_index(1, words_column) << QVariant("")
      << QVariant("hello");

  QTest::newRow("second chord instrument")
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));
};

void Tester::test_delete_cells_template() {
  QFETCH(const QModelIndex, top_left_index_1);
  QFETCH(const QModelIndex, bottom_right_index_1);
  QFETCH(const QModelIndex, top_left_index_2);
  QFETCH(const QModelIndex, bottom_right_index_2);

  auto old_top_left_data =
      chords_model_pointer->data(top_left_index_1, Qt::EditRole);
  auto old_bottom_right_data =
      chords_model_pointer->data(bottom_right_index_2, Qt::EditRole);

  selector_pointer->select(
      QItemSelection(top_left_index_1, bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(top_left_index_2, bottom_right_index_2),
      QItemSelectionModel::Select);
  delete_action_pointer->trigger();
  clear_selection();

  QCOMPARE(
      chords_model_pointer->data(top_left_index_1, Qt::EditRole).toString(),
      "");
  QCOMPARE(
      chords_model_pointer->data(bottom_right_index_2, Qt::EditRole).toString(),
      "");
  undo_stack_pointer->undo();

  QCOMPARE(chords_model_pointer->data(top_left_index_1, Qt::EditRole),
           old_top_left_data);
  QCOMPARE(chords_model_pointer->data(bottom_right_index_2, Qt::EditRole),
           old_bottom_right_data);
}

void Tester::test_delete_cells_template_data() {
  QTest::addColumn<QModelIndex>("top_left_index_1");
  QTest::addColumn<QModelIndex>("bottom_right_index_1");
  QTest::addColumn<QModelIndex>("top_left_index_2");
  QTest::addColumn<QModelIndex>("bottom_right_index_2");

  QTest::newRow("note then chord")
      << chords_model_pointer->get_note_index(0, 1, instrument_column)
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column);

  QTest::newRow("chord then note")
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column)
      << chords_model_pointer->get_note_index(1, 0, instrument_column)
      << chords_model_pointer->get_note_index(1, 0, interval_column);
}

void Tester::test_delete_cells_3_template() {
  QFETCH(const QModelIndex, top_left_index_1);
  QFETCH(const QModelIndex, bottom_right_index_1);
  QFETCH(const QModelIndex, top_left_index_2);
  QFETCH(const QModelIndex, bottom_right_index_2);
  QFETCH(const QModelIndex, top_left_index_3);
  QFETCH(const QModelIndex, bottom_right_index_3);

  auto old_top_left_data =
      chords_model_pointer->data(top_left_index_1, Qt::EditRole);
  auto old_bottom_right_data =
      chords_model_pointer->data(bottom_right_index_3, Qt::EditRole);

  selector_pointer->select(
      QItemSelection(top_left_index_1, bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(top_left_index_2, bottom_right_index_2),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(top_left_index_3, bottom_right_index_3),
      QItemSelectionModel::Select);
  delete_action_pointer->trigger();
  clear_selection();

  QCOMPARE(
      chords_model_pointer->data(top_left_index_1, Qt::EditRole).toString(),
      "");
  QCOMPARE(
      chords_model_pointer->data(bottom_right_index_3, Qt::EditRole).toString(),
      "");
  undo_stack_pointer->undo();

  QCOMPARE(chords_model_pointer->data(top_left_index_1, Qt::EditRole),
           old_top_left_data);
  QCOMPARE(chords_model_pointer->data(bottom_right_index_3, Qt::EditRole),
           old_bottom_right_data);
}

void Tester::test_delete_cells_3_template_data() {
  QTest::addColumn<QModelIndex>("top_left_index_1");
  QTest::addColumn<QModelIndex>("bottom_right_index_1");
  QTest::addColumn<QModelIndex>("top_left_index_2");
  QTest::addColumn<QModelIndex>("bottom_right_index_2");
  QTest::addColumn<QModelIndex>("top_left_index_3");
  QTest::addColumn<QModelIndex>("bottom_right_index_3");

  QTest::newRow("note chord note")
      << chords_model_pointer->get_note_index(0, 1, instrument_column)
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column)
      << chords_model_pointer->get_note_index(1, 0, instrument_column)
      << chords_model_pointer->get_note_index(1, 0, interval_column);

  QTest::newRow("chord notes chord")
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << chords_model_pointer->get_chord_index(0, interval_column)
      << chords_model_pointer->get_note_index(0, 0, instrument_column)
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column);
}

void Tester::test_paste_cell_template() {
  QFETCH(const QModelIndex, old_index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QModelIndex, new_index);
  QFETCH(const QVariant, new_value);

  trigger_action(new_index, SELECT_CELL, copy_action_pointer);

  trigger_action(old_index, SELECT_CELL, paste_cells_or_after_action_pointer);

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
      << chords_model_pointer->get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval())
      << chords_model_pointer->get_chord_index(1, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("chord beats")
      << chords_model_pointer->get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_chord_index(1, beats_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord tempo ratio")
      << chords_model_pointer->get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_chord_index(1, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord volume ratio")
      << chords_model_pointer->get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_chord_index(1, volume_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord words")
      << chords_model_pointer->get_chord_index(0, words_column) << QVariant("")
      << chords_model_pointer->get_chord_index(1, words_column)
      << QVariant("hello");

  QTest::newRow("chord instrument")
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));

  QTest::newRow("note interval")
      << chords_model_pointer->get_note_index(0, 0, interval_column)
      << QVariant::fromValue(Interval())
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("note beats")
      << chords_model_pointer->get_note_index(0, 0, beats_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_note_index(0, 1, beats_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note tempo ratio")
      << chords_model_pointer->get_note_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_note_index(0, 1, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note volume ratio")
      << chords_model_pointer->get_note_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << chords_model_pointer->get_note_index(0, 1, volume_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note words")
      << chords_model_pointer->get_note_index(0, 0, words_column)
      << QVariant("")
      << chords_model_pointer->get_note_index(0, 1, words_column)
      << QVariant("hello");

  QTest::newRow("note instrument")
      << chords_model_pointer->get_note_index(0, 0, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << chords_model_pointer->get_note_index(0, 1, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));
}

void Tester::test_paste_cells_template() {
  QFETCH(const QModelIndex, copy_top_left_index_1);
  QFETCH(const QModelIndex, copy_bottom_right_index_1);
  QFETCH(const QModelIndex, copy_top_left_index_2);
  QFETCH(const QModelIndex, copy_bottom_right_index_2);

  QFETCH(const QModelIndex, paste_top_left_index_1);
  QFETCH(const QModelIndex, paste_bottom_right_index_1);
  QFETCH(const QModelIndex, paste_top_left_index_2);
  QFETCH(const QModelIndex, paste_bottom_right_index_2);

  auto copy_top_left_data =
      chords_model_pointer->data(copy_top_left_index_1, Qt::EditRole);
  auto copy_bottom_right_data =
      chords_model_pointer->data(copy_bottom_right_index_2, Qt::EditRole);

  auto paste_top_left_data =
      chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole);
  auto paste_bottom_right_data =
      chords_model_pointer->data(paste_bottom_right_index_2, Qt::EditRole);

  selector_pointer->select(
      QItemSelection(copy_top_left_index_1, copy_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(copy_top_left_index_2, copy_bottom_right_index_2),
      QItemSelectionModel::Select);
  copy_action_pointer->trigger();
  clear_selection();

  selector_pointer->select(
      QItemSelection(paste_top_left_index_1, paste_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(paste_top_left_index_2, paste_bottom_right_index_2),
      QItemSelectionModel::Select);
  paste_cells_or_after_action_pointer->trigger();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           copy_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_2, Qt::EditRole),
           copy_bottom_right_data);
  undo_stack_pointer->undo();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           paste_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_2, Qt::EditRole),
           paste_bottom_right_data);
}

void Tester::test_paste_cells_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_left_index_1");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("copy_top_left_index_2");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_2");
  QTest::addColumn<QModelIndex>("paste_top_left_index_1");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("paste_top_left_index_2");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_2");

  QTest::newRow("note then chord")
      << chords_model_pointer->get_note_index(0, 1, instrument_column)
      << chords_model_pointer->get_note_index(0, 1, interval_column)
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column)
      << chords_model_pointer->get_note_index(1, 0, instrument_column)
      << chords_model_pointer->get_note_index(1, 0, interval_column)
      << chords_model_pointer->get_chord_index(2, instrument_column)
      << chords_model_pointer->get_chord_index(2, interval_column);

  QTest::newRow("chord then note")
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << chords_model_pointer->get_chord_index(0, interval_column)
      << chords_model_pointer->get_note_index(0, 0, instrument_column)
      << chords_model_pointer->get_note_index(0, 0, interval_column)
      << chords_model_pointer->get_chord_index(1, instrument_column)
      << chords_model_pointer->get_chord_index(1, interval_column)
      << chords_model_pointer->get_note_index(1, 0, instrument_column)
      << chords_model_pointer->get_note_index(1, 0, interval_column);
}

void Tester::test_paste_wrong_cell_template() {
  QFETCH(const QModelIndex, old_index);
  QFETCH(const QModelIndex, new_index);
  QFETCH(const QString, error_message);

  trigger_action(old_index, SELECT_CELL, copy_action_pointer);

  close_message_later(error_message);
  trigger_action(new_index, SELECT_CELL, paste_cells_or_after_action_pointer);
}

void Tester::test_paste_wrong_cell_template_data() {
  QTest::addColumn<QModelIndex>("old_index");
  QTest::addColumn<QModelIndex>("new_index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("beats to interval")
      << chords_model_pointer->get_chord_index(0, interval_column)
      << chords_model_pointer->get_chord_index(0, beats_column)
      << "Destination left column Beats doesn't match pasted left column "
         "Interval";
}

void Tester::test_paste_too_many_template() {
  QFETCH(const QModelIndex, copy_top_left_index_1);
  QFETCH(const QModelIndex, copy_bottom_right_index_1);
  QFETCH(const QModelIndex, copy_top_left_index_2);
  QFETCH(const QModelIndex, copy_bottom_right_index_2);
  QFETCH(const QModelIndex, paste_top_left_index);
  QFETCH(const QString, error_message);

  selector_pointer->select(
      QItemSelection(copy_top_left_index_1, copy_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(copy_top_left_index_2, copy_bottom_right_index_2),
      QItemSelectionModel::Select);
  copy_action_pointer->trigger();
  clear_selection();

  close_message_later(error_message);
  trigger_action(paste_top_left_index, SELECT_CELL,
                 paste_cells_or_after_action_pointer);
};

void Tester::test_paste_too_many_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_left_index_1");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("copy_top_left_index_2");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_2");
  QTest::addColumn<QModelIndex>("paste_top_left_index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("first two")
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << chords_model_pointer->get_chord_index(0, interval_column)
      << chords_model_pointer->get_note_index(0, 0, instrument_column)
      << chords_model_pointer->get_note_index(0, 0, interval_column)
      << chords_model_pointer->get_chord_index(2, instrument_column)
      << "Pasted 2 rows but only 1 row(s) available";
};

void Tester::test_insert_delete() const {
  trigger_action(chords_model_pointer->get_chord_index(2), SELECT_ROWS,
                 insert_into_action_pointer);
  QCOMPARE(
      chords_model_pointer->rowCount(chords_model_pointer->get_chord_index(2)),
      1);
  undo_stack_pointer->undo();
  QCOMPARE(
      chords_model_pointer->rowCount(chords_model_pointer->get_chord_index(2)),
      0);

  // test chord templating from previous chord
  trigger_action(chords_model_pointer->get_chord_index(1), SELECT_ROWS,
                 insert_after_action_pointer);
  QCOMPARE(
      chords_model_pointer->data(
          chords_model_pointer->get_chord_index(1, beats_column), Qt::EditRole),
      QVariant::fromValue(Rational(2, 2)));
  undo_stack_pointer->undo();

  // test note templating from previous note
  trigger_action(chords_model_pointer->get_note_index(0, 1), SELECT_ROWS,
                 insert_after_action_pointer);
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(0, 2, beats_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(0, 2, volume_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(0, 2, tempo_ratio_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(0, 2, words_column),
               Qt::EditRole),
           "hello");
  undo_stack_pointer->undo();

  // test note inheritance from chord
  trigger_action(chords_model_pointer->get_chord_index(1), SELECT_ROWS,
                 insert_into_action_pointer);
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(1, 0, beats_column),
               Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               chords_model_pointer->get_note_index(1, 0, words_column),
               Qt::EditRole),
           "hello");
  undo_stack_pointer->undo();
}

void Tester::test_insert_delete_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(QAction *, action_pointer);
  QFETCH(const int, old_row_count);
  QFETCH(const int, new_row_count);

  auto parent_index = chords_model_pointer->parent(index);

  trigger_action(index, SELECT_ROWS, action_pointer);
  QCOMPARE(chords_model_pointer->rowCount(parent_index), new_row_count);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_insert_delete_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QAction *>("action_pointer");
  QTest::addColumn<int>("old_row_count");
  QTest::addColumn<int>("new_row_count");

  QTest::newRow("insert chord after")
      << chords_model_pointer->get_chord_index(0) << insert_after_action_pointer
      << 3 << 4;
  QTest::newRow("delete chord") << chords_model_pointer->get_chord_index(0)
                                << delete_action_pointer << 3 << 2;

  QTest::newRow("insert note after")
      << chords_model_pointer->get_note_index(0, 0)
      << insert_after_action_pointer << 2 << 3;
  QTest::newRow("delete note") << chords_model_pointer->get_note_index(0, 0)
                               << delete_action_pointer << 2 << 1;
}

void Tester::test_paste_rows_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const int, parent_row_count);

  auto parent_index = chords_model_pointer->parent(index);

  trigger_action(index, SELECT_ROWS, copy_action_pointer);
  trigger_action(index, SELECT_ROWS, paste_cells_or_after_action_pointer);

  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count + 1);
  undo_stack_pointer->undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count);
}

void Tester::test_paste_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("parent_row_count");

  QTest::newRow("paste chord after")
      << chords_model_pointer->get_chord_index(0) << 3;
  QTest::newRow("paste note after")
      << chords_model_pointer->get_note_index(0, 0) << 2;
}

void Tester::test_bad_paste_template() {
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QModelIndex, index);
  QFETCH(const QItemSelectionModel::SelectionFlags, flags);
  QFETCH(QAction *, action_pointer);
  QFETCH(const QString, error_message);

  close_message_later(error_message);
  copy_text(copied.toStdString(), mime_type);
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
      << "[" << "application/prs.chords+json"
      << chords_model_pointer->get_chord_index(0) << SELECT_ROWS
      << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("empty chords")
      << "[]" << "application/prs.chords+json"
      << chords_model_pointer->get_chord_index(0) << SELECT_ROWS
      << paste_cells_or_after_action_pointer
      << "Nothing to paste!";

  QTest::newRow("wrong type chord")
      << "{\"a\": 1}" << "application/prs.chords+json"
      << chords_model_pointer->get_chord_index(0) << SELECT_ROWS
      << paste_cells_or_after_action_pointer
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("unparsable note")
      << "[" << "application/prs.notes+json"
      << chords_model_pointer->get_note_index(0, 0) << SELECT_ROWS
      << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong type note")
      << "{\"a\": 1}" << "application/prs.notes+json"
      << chords_model_pointer->get_note_index(0, 0) << SELECT_ROWS
      << paste_cells_or_after_action_pointer
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("wrong row mime type")
      << "{\"a\": 1}" << "not a mime" << chords_model_pointer->get_chord_index(0)
      << SELECT_ROWS << paste_cells_or_after_action_pointer
      << "Cannot paste not a mime into destination needing chords";

  QTest::newRow("wrong cell mime type")
      << "{\"a\": 1}" << "not a mime"
      << chords_model_pointer->get_chord_index(0, interval_column)
      << SELECT_CELL << paste_cells_or_after_action_pointer
      << "Cannot paste not a mime into destination needing cells";

  QTest::newRow("unparsable interval")
      << "[" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, interval_column)
      << SELECT_CELL << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable rational")
      << "[" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, beats_column) << SELECT_CELL
      << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable instrument")
      << "[" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << SELECT_CELL << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable octave")
      << "[" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, words_column) << SELECT_CELL
      << paste_cells_or_after_action_pointer
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong interval type")
      << "[1]" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, interval_column)
      << SELECT_CELL << paste_cells_or_after_action_pointer
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong rational type")
      << "[1]" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, beats_column) << SELECT_CELL
      << paste_cells_or_after_action_pointer
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong instrument type")
      << "[1]" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, instrument_column)
      << SELECT_CELL << paste_cells_or_after_action_pointer
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong words type")
      << "[1]" << "application/prs.cells+json"
      << chords_model_pointer->get_chord_index(0, words_column) << SELECT_CELL
      << paste_cells_or_after_action_pointer
      << "At  of [1] - unexpected instance type\n";
}

void Tester::test_paste_rows() {
  // copy chord
  trigger_action(chords_model_pointer->get_chord_index(0), SELECT_ROWS,
                 copy_action_pointer);

  // can't paste chord as a note
  close_message_later("Cannot paste chords into destination needing notes");
  trigger_action(chords_model_pointer->get_note_index(0, 0), SELECT_ROWS,
                 paste_cells_or_after_action_pointer);

  // copy note
  trigger_action(chords_model_pointer->get_note_index(0, 0), SELECT_ROWS,
                 copy_action_pointer);

  // paste note into
  trigger_action(chords_model_pointer->get_chord_index(2), SELECT_ROWS,
                 paste_into_action_pointer);
  QCOMPARE(
      chords_model_pointer->rowCount(chords_model_pointer->get_chord_index(2)),
      1);
  undo_stack_pointer->undo();
  QCOMPARE(
      chords_model_pointer->rowCount(chords_model_pointer->get_chord_index(2)),
      0);

  // can't paste note as chord
  close_message_later("Cannot paste notes into destination needing chords");
  trigger_action(chords_model_pointer->get_chord_index(0), SELECT_ROWS,
                 paste_cells_or_after_action_pointer);
}

void Tester::test_play() {
  // Test volume errors
  QVERIFY(chords_model_pointer->setData(
      chords_model_pointer->get_note_index(0, 0, volume_ratio_column),
      QVariant::fromValue(Rational(10)), Qt::EditRole));

  close_message_later(
      "Volume exceeds 100% for chord 1, note 1. Playing with 100% volume.");
  trigger_action(chords_model_pointer->get_note_index(0, 0), SELECT_ROWS,
                 play_action_pointer);
  QThread::msleep(WAIT_TIME);
  stop_playing_action_pointer->trigger();
  undo_stack_pointer->undo();

  // Test midi overload
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    trigger_action(chords_model_pointer->get_chord_index(0), SELECT_ROWS,
                   insert_into_action_pointer);
  }

  close_message_later(
      "Out of MIDI channels for chord 1, note 17. Not playing note.");
  trigger_action(chords_model_pointer->get_chord_index(0), SELECT_ROWS,
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

  QTest::newRow("first two chords") << chords_model_pointer->get_chord_index(0)
                                    << chords_model_pointer->get_chord_index(1);
  QTest::newRow("second chord") << chords_model_pointer->get_chord_index(1)
                                << chords_model_pointer->get_chord_index(1);

  auto first_chord_second_note_index =
      chords_model_pointer->get_note_index(0, 1);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index =
      chords_model_pointer->get_note_index(1, 0);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_io() {

  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  auto file_name = temp_json_file.fileName();
  song_editor.save_as_file(file_name);

  QCOMPARE(song_editor.current_file, file_name);

  QVERIFY(temp_json_file.open());
  auto written = QString(temp_json_file.readAll());
  temp_json_file.close();
// different encoding on windows or something
#ifndef _WIN32
  QCOMPARE(written, SONG_TEXT);
#endif
  save_action_pointer->trigger();

  song_editor.export_to_file(file_name);

  QTemporaryFile broken_json_file;
  broken_json_file.open();
  broken_json_file.write("{");
  broken_json_file.close();
  close_message_later(
      "[json.exception.parse_error.101] parse error at line 1, column 2: "
      "syntax error while parsing object key - unexpected end of input; "
      "expected string literal");
  song_editor.open_file(broken_json_file.fileName());

  QTemporaryFile wrong_type_json_file;
  wrong_type_json_file.open();
  wrong_type_json_file.write("[1]");
  wrong_type_json_file.close();
  close_message_later("At  of [1] - unexpected instance type\n");
  song_editor.open_file(wrong_type_json_file.fileName());

  QTemporaryFile working_json_file;
  working_json_file.open();
  working_json_file.write(R""""({
    "starting_instrument": "Marimba",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_volume_percent": 50
})"""");
  working_json_file.close();
  song_editor.open_file(working_json_file.fileName());

  QCOMPARE(chords_model_pointer->rowCount(QModelIndex()), 0);
}