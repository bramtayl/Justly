#include "tests/Tester.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QString>
#include <QTemporaryFile>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QTreeView>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <memory>
#include <string>

#include "justly/Instrument.hpp"
#include "justly/Interval.hpp"
#include "justly/NoteChordColumn.hpp"
#include "justly/Rational.hpp"
#include "justly/SongEditor.hpp"

const auto STARTING_KEY_1 = 401.0;
const auto STARTING_KEY_2 = 402.0;

const auto STARTING_TEMPO_1 = 150.0;
const auto STARTING_TEMPO_2 = 100.0;

const auto STARTING_VELOCITY_1 = 70;
const auto STARTING_VELOCITY_2 = 80;

const auto WAIT_TIME = 500;

const auto OVERLOAD_NUMBER = 15;

const auto NEW_GAIN_1 = 2;
const auto NEW_GAIN_2 = 3;

const auto *const SONG_TEXT = R""""({
    "chords": [
        {
            "notes": [
                {},
                {}
            ]
        },
        {
            "notes": [
                {},
                {}
            ]
        },
        {
            "beats": {
                "numerator": 2
            },
            "instrument": "12-String Guitar",
            "interval": {
                "octave": 1
            },
            "notes": [
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "hi"
                },
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "hi"
                }
            ],
            "tempo_ratio": {
                "numerator": 2
            },
            "velocity_ratio": {
                "denominator": 2
            },
            "words": "hi"
        },
        {
            "beats": {
                "numerator": 2
            },
            "instrument": "12-String Guitar",
            "interval": {
                "octave": 1
            },
            "notes": [
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "hi"
                },
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "hi"
                }
            ],
            "tempo_ratio": {
                "numerator": 2
            },
            "velocity_ratio": {
                "denominator": 2
            },
            "words": "hi"
        }
    ],
    "gain": 1.0,
    "starting_instrument": "Cello",
    "starting_key": 220.0,
    "starting_tempo": 100.0,
    "starting_velocity": 64.0
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

void Tester::open_text(const QString &json_song) {
  QTemporaryFile json_file;
  if (json_file.open()) {
    json_file.write(json_song.toStdString().c_str());
    json_file.close();
  }
  song_editor.open_file(json_file.fileName());
}

Tester::Tester()
    : song_editor({}),
      chords_view_pointer(song_editor.get_chords_view_pointer()),
      selector_pointer(chords_view_pointer->selectionModel()),
      chords_model_pointer(chords_view_pointer->model()) {}

void Tester::initTestCase() {
  register_converters();
  open_text(SONG_TEXT);
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

  QTest::newRow("song") << QModelIndex() << 4;
  QTest::newRow("first chord") << song_editor.get_chord_index(0) << 2;
  QTest::newRow("second chord") << song_editor.get_chord_index(1) << 2;
  QTest::newRow("third chord") << song_editor.get_chord_index(2) << 2;
  QTest::newRow("fourth chord") << song_editor.get_chord_index(3) << 2;
  QTest::newRow("non-symbol chord")
      << song_editor.get_chord_index(0, interval_column) << 0;
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
      << song_editor.get_chord_index(0) << QModelIndex();
  QTest::newRow("note parent")
      << song_editor.get_note_index(0, 0) << song_editor.get_chord_index(0);
}

void Tester::test_column_count() const {
  QCOMPARE(chords_model_pointer->columnCount(QModelIndex()), 7);
}

void Tester::test_gain_control() {
  auto old_gain = song_editor.get_gain();
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  song_editor.set_gain(NEW_GAIN_1);
  QCOMPARE(song_editor.get_gain(), NEW_GAIN_1);
  song_editor.set_gain(NEW_GAIN_2);
  QCOMPARE(song_editor.get_gain(), NEW_GAIN_2);

  song_editor.undo();
  QCOMPARE(song_editor.get_gain(), old_gain);
}

void Tester::test_starting_instrument_control() const {
  const auto *old_instrument = song_editor.get_starting_instrument();
  const auto *new_instrument_1 = get_instrument_pointer("Oboe");
  const auto *new_instrument_2 = get_instrument_pointer("Ocarina");

  QCOMPARE_NE(old_instrument, new_instrument_1);
  QCOMPARE_NE(old_instrument, new_instrument_2);

  // test combining
  song_editor.set_starting_instrument(new_instrument_1);
  QCOMPARE(song_editor.get_starting_instrument(), new_instrument_1);
  song_editor.set_starting_instrument(new_instrument_2);
  QCOMPARE(song_editor.get_starting_instrument(), new_instrument_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_instrument(), old_instrument);
}

void Tester::test_starting_key_control() const {
  auto old_key = song_editor.get_starting_key();
  QCOMPARE_NE(old_key, STARTING_KEY_1);
  QCOMPARE_NE(old_key, STARTING_KEY_2);

  // test combining
  song_editor.set_starting_key(STARTING_KEY_1);
  QCOMPARE(song_editor.get_starting_key(), STARTING_KEY_1);
  song_editor.set_starting_key(STARTING_KEY_2);
  QCOMPARE(song_editor.get_starting_key(), STARTING_KEY_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_key(), old_key);
}

void Tester::test_starting_velocity_control() const {
  auto old_velocity = song_editor.get_starting_velocity();
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_1);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_2);

  // test combining
  song_editor.set_starting_velocity(STARTING_VELOCITY_1);
  QCOMPARE(song_editor.get_starting_velocity(), STARTING_VELOCITY_1);
  song_editor.set_starting_velocity(STARTING_VELOCITY_2);
  QCOMPARE(song_editor.get_starting_velocity(), STARTING_VELOCITY_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_velocity(), old_velocity);
}

void Tester::test_starting_tempo_control() const {
  auto old_tempo = song_editor.get_starting_tempo();

  // test combining
  song_editor.set_starting_tempo(STARTING_TEMPO_1);
  QCOMPARE(song_editor.get_starting_tempo(), STARTING_TEMPO_1);
  song_editor.set_starting_tempo(STARTING_TEMPO_2);
  QCOMPARE(song_editor.get_starting_tempo(), STARTING_TEMPO_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_tempo(), old_tempo);
}

void Tester::test_column_headers_template() const {
  QFETCH(const NoteChordColumn, field);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);

  QCOMPARE(chords_model_pointer->headerData(field, orientation, role), value);
}

void Tester::test_column_headers_template_data() {
  QTest::addColumn<NoteChordColumn>("field");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("type") << type_column << Qt::Horizontal << Qt::DisplayRole
                        << QVariant("Type");
  QTest::newRow("interval") << interval_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Interval");
  QTest::newRow("beats") << beats_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Beats");
  QTest::newRow("velocity") << velocity_ratio_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Velocity ratio");
  QTest::newRow("tempo") << tempo_ratio_column << Qt::Horizontal
                         << Qt::DisplayRole << QVariant("Tempo ratio");
  QTest::newRow("words") << words_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Words");
  QTest::newRow("instruments") << instrument_column << Qt::Horizontal
                               << Qt::DisplayRole << QVariant("Instrument");
  QTest::newRow("wrong orientation")
      << type_column << Qt::Vertical << Qt::DisplayRole << QVariant();
  QTest::newRow("wrong role")
      << type_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
}

void Tester::test_select_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, second_index);
  selector_pointer->select(first_index, QItemSelectionModel::Select);
  selector_pointer->select(second_index, QItemSelectionModel::Select);
  auto selected_rows = selector_pointer->selectedRows();
  QCOMPARE(selected_rows.size(), 1);
  QCOMPARE(selected_rows[0], first_index);
  clear_selection();
}

void Tester::test_select_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("second_index");

  auto first_chord_index = song_editor.get_chord_index(0);
  auto first_note_index = song_editor.get_note_index(0, 0);

  QTest::newRow("chord then note") << first_chord_index << first_note_index;
  QTest::newRow("note then chord") << first_note_index << first_chord_index;
}

void Tester::test_flags_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemFlags, item_flags);
  QCOMPARE(chords_model_pointer->flags(index), item_flags);
}

void Tester::test_flags_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemFlags>("item_flags");

  QTest::newRow("first chord symbol")
      << song_editor.get_chord_index(0)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first chord symbol")
      << song_editor.get_chord_index(0, interval_column)
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
  QTest::newRow("first chord symbol")
      << song_editor.get_chord_index(0) << Qt::DisplayRole << QVariant("♫");
  QTest::newRow("first chord decoration")
      << song_editor.get_chord_index(0) << Qt::DecorationRole << QVariant();
  QTest::newRow("first note symbol")
      << song_editor.get_note_index(0, 0) << Qt::DisplayRole << QVariant("♪");
  QTest::newRow("first note decoration")
      << song_editor.get_note_index(0, 0) << Qt::DecorationRole << QVariant();
}

void Tester::test_background() const {
  QCOMPARE_NE(chords_model_pointer->data(song_editor.get_chord_index(0),
                                         Qt::BackgroundRole),
              chords_model_pointer->data(song_editor.get_note_index(0, 0),
                                         Qt::BackgroundRole));
}

void Tester::test_delegate_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, new_value);

  auto old_value = chords_model_pointer->data(index, Qt::EditRole);
  QCOMPARE_NE(old_value, new_value);

  auto *cell_editor_pointer = song_editor.create_editor(index);

  QCOMPARE(cell_editor_pointer->property(
               cell_editor_pointer->metaObject()->userProperty().name()),
           old_value);

  song_editor.set_editor(cell_editor_pointer, index, new_value);

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
}

void Tester::test_delegate_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("instrument")
      << song_editor.get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval") << song_editor.get_chord_index(0, interval_column)
                            << QVariant::fromValue(Interval(2));
  QTest::newRow("beats") << song_editor.get_chord_index(0, beats_column)
                         << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("velocity")
      << song_editor.get_chord_index(0, velocity_ratio_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("tempo") << song_editor.get_chord_index(0, tempo_ratio_column)
                         << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("words") << song_editor.get_chord_index(0, words_column)
                         << QVariant("hello");
}

void Tester::test_no_set_value() const {
  // setData only works for the edit role
  QVERIFY(!(chords_model_pointer->setData(song_editor.get_chord_index(0),
                                          QVariant(), Qt::DecorationRole)));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, new_value);

  auto old_value = chords_model_pointer->data(index, Qt::EditRole);
  QCOMPARE_NE(old_value, new_value);

  QVERIFY(chords_model_pointer->setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), new_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);

  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), old_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("first_chord_interval")
      << song_editor.get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval(2));
  QTest::newRow("first_chord_beats")
      << song_editor.get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_velocity")
      << song_editor.get_chord_index(0, velocity_ratio_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_tempo")
      << song_editor.get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_words")
      << song_editor.get_chord_index(0, words_column) << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << song_editor.get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("first_note_interval")
      << song_editor.get_note_index(0, 0, interval_column)
      << QVariant::fromValue(Interval(2));
  QTest::newRow("first_note_beats")
      << song_editor.get_note_index(0, 0, beats_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_velocity")
      << song_editor.get_note_index(0, 0, velocity_ratio_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_tempo")
      << song_editor.get_note_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_words")
      << song_editor.get_note_index(0, 0, words_column) << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << song_editor.get_note_index(0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
}

void Tester::test_delete_cell_template() {
  QFETCH(const QModelIndex, index);

  auto old_value = chords_model_pointer->data(index, Qt::EditRole);
  QVERIFY(!old_value.toString().isEmpty());

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_delete();
  clear_selection();

  QVERIFY(chords_model_pointer->data(index, Qt::EditRole).toString().isEmpty());
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
};

void Tester::test_delete_cell_template_data() {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("third chord interval")
      << song_editor.get_chord_index(2, interval_column);

  QTest::newRow("third chord beats")
      << song_editor.get_chord_index(2, beats_column);

  QTest::newRow("third chord words")
      << song_editor.get_chord_index(2, words_column);

  QTest::newRow("third chord instrument")
      << song_editor.get_chord_index(2, instrument_column);
};

void Tester::test_delete_3_groups_template() {
  QFETCH(const QModelIndex, top_left_index_1);
  QFETCH(const QModelIndex, bottom_right_index_1);
  QFETCH(const QModelIndex, top_left_index_2);
  QFETCH(const QModelIndex, bottom_right_index_2);
  QFETCH(const QModelIndex, top_left_index_3);
  QFETCH(const QModelIndex, bottom_right_index_3);

  auto top_left_data_1 =
      chords_model_pointer->data(top_left_index_1, Qt::EditRole);
  auto bottom_right_data_1 =
      chords_model_pointer->data(bottom_right_index_1, Qt::EditRole);
  auto top_left_data_2 =
      chords_model_pointer->data(top_left_index_2, Qt::EditRole);
  auto bottom_right_data_2 =
      chords_model_pointer->data(bottom_right_index_2, Qt::EditRole);
  auto top_left_data_3 =
      chords_model_pointer->data(top_left_index_3, Qt::EditRole);
  auto bottom_right_data_3 =
      chords_model_pointer->data(bottom_right_index_3, Qt::EditRole);

  QVERIFY(!chords_model_pointer->data(top_left_index_1, Qt::EditRole)
               .toString()
               .isEmpty());
  QVERIFY(!chords_model_pointer->data(bottom_right_index_1, Qt::EditRole)
               .toString()
               .isEmpty());
  QVERIFY(!chords_model_pointer->data(top_left_index_2, Qt::EditRole)
               .toString()
               .isEmpty());
  QVERIFY(!chords_model_pointer->data(bottom_right_index_2, Qt::EditRole)
               .toString()
               .isEmpty());
  QVERIFY(!chords_model_pointer->data(top_left_index_3, Qt::EditRole)
               .toString()
               .isEmpty());
  QVERIFY(!chords_model_pointer->data(bottom_right_index_3, Qt::EditRole)
               .toString()
               .isEmpty());

  selector_pointer->select(
      QItemSelection(top_left_index_1, bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(top_left_index_2, bottom_right_index_2),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(top_left_index_3, bottom_right_index_3),
      QItemSelectionModel::Select);
  song_editor.trigger_delete();
  clear_selection();

  QVERIFY(chords_model_pointer->data(top_left_index_1, Qt::EditRole)
              .toString()
              .isEmpty());
  QVERIFY(chords_model_pointer->data(bottom_right_index_1, Qt::EditRole)
              .toString()
              .isEmpty());
  QVERIFY(chords_model_pointer->data(top_left_index_2, Qt::EditRole)
              .toString()
              .isEmpty());
  QVERIFY(chords_model_pointer->data(bottom_right_index_2, Qt::EditRole)
              .toString()
              .isEmpty());
  QVERIFY(chords_model_pointer->data(top_left_index_3, Qt::EditRole)
              .toString()
              .isEmpty());
  QVERIFY(chords_model_pointer->data(bottom_right_index_3, Qt::EditRole)
              .toString()
              .isEmpty());
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(top_left_index_1, Qt::EditRole),
           top_left_data_1);
  QCOMPARE(chords_model_pointer->data(bottom_right_index_1, Qt::EditRole),
           bottom_right_data_1);
  QCOMPARE(chords_model_pointer->data(top_left_index_2, Qt::EditRole),
           top_left_data_2);
  QCOMPARE(chords_model_pointer->data(bottom_right_index_2, Qt::EditRole),
           bottom_right_data_2);
  QCOMPARE(chords_model_pointer->data(top_left_index_3, Qt::EditRole),
           top_left_data_3);
  QCOMPARE(chords_model_pointer->data(bottom_right_index_3, Qt::EditRole),
           bottom_right_data_3);
}

void Tester::test_delete_3_groups_template_data() {
  QTest::addColumn<QModelIndex>("top_left_index_1");
  QTest::addColumn<QModelIndex>("bottom_right_index_1");
  QTest::addColumn<QModelIndex>("top_left_index_2");
  QTest::addColumn<QModelIndex>("bottom_right_index_2");
  QTest::addColumn<QModelIndex>("top_left_index_3");
  QTest::addColumn<QModelIndex>("bottom_right_index_3");

  QTest::newRow("note chord note")
      << song_editor.get_note_index(2, 1, instrument_column)
      << song_editor.get_note_index(2, 1, interval_column)
      << song_editor.get_chord_index(3, instrument_column)
      << song_editor.get_chord_index(3, interval_column)
      << song_editor.get_note_index(3, 0, instrument_column)
      << song_editor.get_note_index(3, 0, interval_column);

  QTest::newRow("chord notes chord")
      << song_editor.get_chord_index(2, instrument_column)
      << song_editor.get_chord_index(2, interval_column)
      << song_editor.get_note_index(2, 0, instrument_column)
      << song_editor.get_note_index(2, 1, interval_column)
      << song_editor.get_chord_index(3, instrument_column)
      << song_editor.get_chord_index(3, interval_column);
}

void Tester::test_paste_cell_template() {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_value = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);

  QCOMPARE_NE(copy_value, paste_value);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), copy_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
}

void Tester::test_paste_cell_template_data() {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << song_editor.get_chord_index(0, interval_column)
      << song_editor.get_chord_index(2, interval_column);

  QTest::newRow("chord beats") << song_editor.get_chord_index(0, beats_column)
                               << song_editor.get_chord_index(2, beats_column);

  QTest::newRow("chord tempo ratio")
      << song_editor.get_chord_index(0, tempo_ratio_column)
      << song_editor.get_chord_index(2, tempo_ratio_column);

  QTest::newRow("chord velocity ratio")
      << song_editor.get_chord_index(0, velocity_ratio_column)
      << song_editor.get_chord_index(2, velocity_ratio_column);

  QTest::newRow("chord words") << song_editor.get_chord_index(0, words_column)
                               << song_editor.get_chord_index(2, words_column);

  QTest::newRow("chord instrument")
      << song_editor.get_chord_index(0, instrument_column)
      << song_editor.get_chord_index(2, instrument_column);

  QTest::newRow("note interval")
      << song_editor.get_note_index(0, 0, interval_column)
      << song_editor.get_note_index(2, 0, interval_column);

  QTest::newRow("note beats") << song_editor.get_note_index(0, 0, beats_column)
                              << song_editor.get_note_index(2, 0, beats_column);

  QTest::newRow("note tempo ratio")
      << song_editor.get_note_index(0, 0, tempo_ratio_column)
      << song_editor.get_note_index(2, 0, tempo_ratio_column);

  QTest::newRow("note velocity ratio")
      << song_editor.get_note_index(0, 0, velocity_ratio_column)
      << song_editor.get_note_index(2, 0, velocity_ratio_column);

  QTest::newRow("note words") << song_editor.get_note_index(0, 0, words_column)
                              << song_editor.get_note_index(2, 0, words_column);

  QTest::newRow("note instrument")
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(2, 0, instrument_column);
}

void Tester::test_cut_paste_cell_template() {
  QFETCH(const QModelIndex, cut_index);
  QFETCH(const QModelIndex, paste_index);

  const auto cut_value = chords_model_pointer->data(cut_index);
  const auto paste_value = chords_model_pointer->data(paste_index);

  QVERIFY(!cut_value.toString().isEmpty());
  QCOMPARE_NE(cut_value, paste_value);

  selector_pointer->select(cut_index, QItemSelectionModel::Select);
  song_editor.trigger_cut();
  clear_selection();

  QVERIFY(chords_model_pointer->data(cut_index).toString().isEmpty());

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), cut_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(cut_index, Qt::EditRole), cut_value);
}

void Tester::test_cut_paste_cell_template_data() {
  QTest::addColumn<QModelIndex>("cut_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << song_editor.get_chord_index(2, interval_column)
      << song_editor.get_chord_index(0, interval_column);
}

void Tester::test_paste_2_groups_template() {
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

  QCOMPARE_NE(copy_top_left_data, paste_top_left_data);
  QCOMPARE_NE(copy_bottom_right_data, paste_bottom_right_data);

  selector_pointer->select(
      QItemSelection(copy_top_left_index_1, copy_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(copy_top_left_index_2, copy_bottom_right_index_2),
      QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(
      QItemSelection(paste_top_left_index_1, paste_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(paste_top_left_index_2, paste_bottom_right_index_2),
      QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           copy_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_2, Qt::EditRole),
           copy_bottom_right_data);
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           paste_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_2, Qt::EditRole),
           paste_bottom_right_data);
}

void Tester::test_paste_2_groups_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_left_index_1");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("copy_top_left_index_2");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_2");
  QTest::addColumn<QModelIndex>("paste_top_left_index_1");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("paste_top_left_index_2");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_2");

  QTest::newRow("note then chord")
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_chord_index(2, instrument_column)
      << song_editor.get_chord_index(2, interval_column)
      << song_editor.get_note_index(2, 0, instrument_column)
      << song_editor.get_note_index(2, 0, interval_column);

  QTest::newRow("chord then note")
      << song_editor.get_chord_index(0, instrument_column)
      << song_editor.get_chord_index(0, interval_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 0, interval_column)
      << song_editor.get_note_index(2, 1, instrument_column)
      << song_editor.get_note_index(2, 1, interval_column)
      << song_editor.get_chord_index(3, instrument_column)
      << song_editor.get_chord_index(3, interval_column);
}

void Tester::test_paste_3_groups_template() {
  QFETCH(const QModelIndex, copy_top_left_index_1);
  QFETCH(const QModelIndex, copy_bottom_right_index_1);
  QFETCH(const QModelIndex, copy_top_left_index_2);
  QFETCH(const QModelIndex, copy_bottom_right_index_2);
  QFETCH(const QModelIndex, copy_top_left_index_3);
  QFETCH(const QModelIndex, copy_bottom_right_index_3);

  QFETCH(const QModelIndex, paste_top_left_index_1);
  QFETCH(const QModelIndex, paste_bottom_right_index_1);
  QFETCH(const QModelIndex, paste_top_left_index_2);
  QFETCH(const QModelIndex, paste_bottom_right_index_2);
  QFETCH(const QModelIndex, paste_top_left_index_3);
  QFETCH(const QModelIndex, paste_bottom_right_index_3);

  auto copy_top_left_data =
      chords_model_pointer->data(copy_top_left_index_1, Qt::EditRole);
  auto copy_bottom_right_data =
      chords_model_pointer->data(copy_bottom_right_index_3, Qt::EditRole);

  auto paste_top_left_data =
      chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole);
  auto paste_bottom_right_data =
      chords_model_pointer->data(paste_bottom_right_index_3, Qt::EditRole);

  QCOMPARE_NE(copy_top_left_data, paste_top_left_data);
  QCOMPARE_NE(copy_bottom_right_data, paste_bottom_right_data);

  selector_pointer->select(
      QItemSelection(copy_top_left_index_1, copy_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(copy_top_left_index_2, copy_bottom_right_index_2),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(copy_top_left_index_3, copy_bottom_right_index_3),
      QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(
      QItemSelection(paste_top_left_index_1, paste_bottom_right_index_1),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(paste_top_left_index_2, paste_bottom_right_index_2),
      QItemSelectionModel::Select);
  selector_pointer->select(
      QItemSelection(paste_top_left_index_3, paste_bottom_right_index_3),
      QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           copy_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_3, Qt::EditRole),
           copy_bottom_right_data);
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           paste_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_3, Qt::EditRole),
           paste_bottom_right_data);
}

void Tester::test_paste_3_groups_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_left_index_1");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("copy_top_left_index_2");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_2");
  QTest::addColumn<QModelIndex>("copy_top_left_index_3");
  QTest::addColumn<QModelIndex>("copy_bottom_right_index_3");
  QTest::addColumn<QModelIndex>("paste_top_left_index_1");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_1");
  QTest::addColumn<QModelIndex>("paste_top_left_index_2");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_2");
  QTest::addColumn<QModelIndex>("paste_top_left_index_3");
  QTest::addColumn<QModelIndex>("paste_bottom_right_index_3");

  QTest::newRow("chord then note then chord")
      << song_editor.get_chord_index(0, instrument_column)
      << song_editor.get_chord_index(0, interval_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(2, 1, instrument_column)
      << song_editor.get_note_index(2, 1, interval_column)
      << song_editor.get_chord_index(3, instrument_column)
      << song_editor.get_chord_index(3, interval_column)
      << song_editor.get_note_index(3, 1, instrument_column)
      << song_editor.get_note_index(3, 1, interval_column);

  QTest::newRow("note then chord then note")
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(1, 0, instrument_column)
      << song_editor.get_note_index(1, 1, interval_column)
      << song_editor.get_chord_index(2, instrument_column)
      << song_editor.get_chord_index(2, interval_column)
      << song_editor.get_note_index(2, 0, instrument_column)
      << song_editor.get_note_index(2, 1, interval_column)
      << song_editor.get_chord_index(3, instrument_column)
      << song_editor.get_chord_index(3, interval_column);
}

void Tester::test_paste_truncate_template() {
  QFETCH(const QModelIndex, copy_top_index);
  QFETCH(const QModelIndex, copy_bottom_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_top_data = chords_model_pointer->data(copy_top_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);

  selector_pointer->select(QItemSelection(copy_top_index, copy_bottom_index),
                           QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole),
           copy_top_data);
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
};

void Tester::test_paste_truncate_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_index");
  QTest::addColumn<QModelIndex>("copy_bottom_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("first notes")
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(3, 1, instrument_column);
};

void Tester::test_paste_recycle_template() {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_top_index);
  QFETCH(const QModelIndex, paste_bottom_index);

  auto copy_data = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_top_data =
      chords_model_pointer->data(paste_top_index, Qt::EditRole);
  auto paste_bottom_data =
      chords_model_pointer->data(paste_bottom_index, Qt::EditRole);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(QItemSelection(paste_top_index, paste_bottom_index),
                           QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_index, Qt::EditRole),
           copy_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_index, Qt::EditRole),
           copy_data);
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(paste_top_index, Qt::EditRole),
           paste_top_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_index, Qt::EditRole),
           paste_bottom_data);
};

void Tester::test_paste_recycle_template_data() {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_top_index");
  QTest::addColumn<QModelIndex>("paste_bottom_index");

  QTest::newRow("first two notes")
      << song_editor.get_note_index(3, 0, instrument_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, instrument_column);
};

void Tester::test_insert_into() const {
  auto chord_index = song_editor.get_chord_index(0);
  auto old_row_count = chords_model_pointer->rowCount(chord_index);
  selector_pointer->select(chord_index, QItemSelectionModel::Select);
  song_editor.trigger_insert_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(chord_index), old_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(chord_index), old_row_count);
}

void Tester::test_new_chord_from_chord() const {
  auto chord_beats_value =
      chords_model_pointer->data(song_editor.get_chord_index(2, beats_column));
  QVERIFY(!chord_beats_value.toString().isEmpty());

  selector_pointer->select(song_editor.get_chord_index(2),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_chord_index(3, beats_column), Qt::EditRole),
           chord_beats_value);
  song_editor.undo();
}

void Tester::test_new_note_from_chord() const {
  auto chord_beats_value =
      chords_model_pointer->data(song_editor.get_chord_index(2, beats_column));
  QVERIFY(!chord_beats_value.toString().isEmpty());

  auto chord_words_value =
      chords_model_pointer->data(song_editor.get_chord_index(2, words_column));
  QVERIFY(!chord_words_value.toString().isEmpty());

  selector_pointer->select(song_editor.get_chord_index(2),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(2, 0, beats_column), Qt::EditRole),
           chord_beats_value);
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(2, 0, words_column), Qt::EditRole),
           chord_words_value);
  song_editor.undo();
}

void Tester::test_new_note_from_note() const {
  auto note_beats_value = chords_model_pointer->data(
      song_editor.get_note_index(2, 0, beats_column));
  QVERIFY(!note_beats_value.toString().isEmpty());

  auto note_velocity_ratio_value = chords_model_pointer->data(
      song_editor.get_note_index(2, 0, velocity_ratio_column));
  QVERIFY(!note_velocity_ratio_value.toString().isEmpty());

  auto note_tempo_ratio_value = chords_model_pointer->data(
      song_editor.get_note_index(2, 0, tempo_ratio_column));
  QVERIFY(!note_tempo_ratio_value.toString().isEmpty());

  auto note_words_value = chords_model_pointer->data(
      song_editor.get_note_index(2, 0, words_column));
  QVERIFY(!note_words_value.toString().isEmpty());

  selector_pointer->select(song_editor.get_note_index(2, 0),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(2, 1, beats_column), Qt::EditRole),
           note_beats_value);
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(2, 1, velocity_ratio_column),
               Qt::EditRole),
           note_velocity_ratio_value);
  QCOMPARE(
      chords_model_pointer->data(
          song_editor.get_note_index(2, 1, tempo_ratio_column), Qt::EditRole),
      note_tempo_ratio_value);
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(2, 1, words_column), Qt::EditRole),
           note_words_value);
  song_editor.undo();
}

void Tester::test_insert_rows_template() {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_insert_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("insert chord after") << song_editor.get_chord_index(0);
  QTest::newRow("insert note after") << song_editor.get_note_index(0, 0);
}

void Tester::test_delete_rows_template() {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_delete();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count - 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_delete_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("delete chord") << song_editor.get_chord_index(0);
  QTest::newRow("delete note") << song_editor.get_note_index(0, 0);
}

void Tester::test_paste_rows_template() {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_paste_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("parent_row_count");

  QTest::newRow("paste chord after") << song_editor.get_chord_index(0);
  QTest::newRow("paste note after") << song_editor.get_note_index(0, 0);
}

void Tester::test_bad_paste_template() {
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QModelIndex, index);
  QFETCH(const QString, error_message);

  close_message_later(error_message);

  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(mime_type, copied.toStdString().c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();
}

void Tester::test_bad_paste_template_data() {
  QTest::addColumn<QString>("copied");
  QTest::addColumn<QString>("mime_type");
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("unparsable chord")
      << "[" << "application/prs.chords+json" << song_editor.get_chord_index(0)

      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("empty chords")
      << "[]" << "application/prs.chords+json" << song_editor.get_chord_index(0)
      << "Nothing to paste!";

  QTest::newRow("wrong type chord")
      << "{\"a\": 1}" << "application/prs.chords+json"
      << song_editor.get_chord_index(0)
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("unparsable note")
      << "[" << "application/prs.notes+json" << song_editor.get_note_index(0, 0)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong type note")
      << "{\"a\": 1}" << "application/prs.notes+json"
      << song_editor.get_note_index(0, 0)
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("wrong row mime type")
      << "{\"a\": 1}" << "not a mime" << song_editor.get_chord_index(0)

      << "Cannot paste not a mime into destination needing chords";

  QTest::newRow("wrong cell mime type")
      << "{\"a\": 1}" << "not a mime"
      << song_editor.get_chord_index(0, interval_column)
      << "Cannot paste not a mime into destination needing cells";

  QTest::newRow("unparsable interval")
      << "[" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, interval_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable rational")
      << "[" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, beats_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable instrument")
      << "[" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, instrument_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable octave")
      << "[" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, words_column)

      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong interval type")
      << "[1]" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, interval_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong rational type")
      << "[1]" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, beats_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong instrument type")
      << "[1]" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, instrument_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong words type")
      << "[1]" << "application/prs.cells+json"
      << song_editor.get_chord_index(0, words_column)
      << "At  of [1] - unexpected instance type\n";
}

void Tester::test_paste_wrong_level_template() {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);
  QFETCH(const QString, error_message);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  close_message_later(error_message);

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();
}

void Tester::test_paste_wrong_level_template_data() {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("chords to notes")
      << song_editor.get_chord_index(0) << song_editor.get_note_index(0, 0)
      << "Cannot paste chords into destination needing notes";

  QTest::newRow("notes to chords")
      << song_editor.get_note_index(0, 0) << song_editor.get_chord_index(0)
      << "Cannot paste notes into destination needing chords";
}

void Tester::test_paste_into() {
  auto note_index = song_editor.get_note_index(0, 0);
  auto parent_index = chords_model_pointer->parent(note_index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);
  selector_pointer->select(note_index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  // paste note into
  selector_pointer->select(parent_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_too_loud() {
  QVERIFY(chords_model_pointer->setData(
      song_editor.get_note_index(0, 0, velocity_ratio_column),
      QVariant::fromValue(Rational(10)), Qt::EditRole));

  close_message_later(
      "Velocity exceeds 127 for chord 1, note 1. Playing with velocity 127.");

  selector_pointer->select(song_editor.get_note_index(0, 0),
                           QItemSelectionModel::Select);
  song_editor.trigger_play();
  clear_selection();

  QThread::msleep(WAIT_TIME);
  song_editor.trigger_stop_playing();
  song_editor.undo();
}

void Tester::test_too_many_channels() {
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    selector_pointer->select(song_editor.get_chord_index(0),
                             QItemSelectionModel::Select);
    song_editor.trigger_insert_into();
    clear_selection();
  }

  close_message_later(
      "Out of MIDI channels for chord 1, note 17. Not playing note.");

  selector_pointer->select(song_editor.get_chord_index(0),
                           QItemSelectionModel::Select);
  song_editor.trigger_play();
  clear_selection();

  QThread::msleep(WAIT_TIME);
  song_editor.trigger_stop_playing();

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    song_editor.undo();
  }
}

void Tester::test_play_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  selector_pointer->select(QItemSelection(first_index, last_index),
                           QItemSelectionModel::Select);
  song_editor.trigger_play();
  // first cut off early
  song_editor.trigger_play();
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  clear_selection();
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << song_editor.get_chord_index(0) << song_editor.get_chord_index(1);
  QTest::newRow("second chord")
      << song_editor.get_chord_index(1) << song_editor.get_chord_index(1);

  auto first_chord_second_note_index = song_editor.get_note_index(0, 1);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index = song_editor.get_note_index(1, 0);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_expand_collapse() const {
  auto index = song_editor.get_chord_index(0);
  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_collapse();
  song_editor.trigger_expand();
  Q_ASSERT(chords_view_pointer->isExpanded(index));
  song_editor.trigger_collapse();
  Q_ASSERT(!chords_view_pointer->isExpanded(index));
};

void Tester::test_file_dialog() {
  QVERIFY(song_editor.make_file_dialog(tr("Open — Justly"),
                                       "JSON file (*.json)",
                                       QFileDialog::AcceptOpen, ".json",
                                       QFileDialog::ExistingFile) != nullptr);
}

void Tester::test_save() {
  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  auto file_name = temp_json_file.fileName();
  song_editor.save_as_file(file_name);

  QCOMPARE(song_editor.get_current_file(), file_name);

  QVERIFY(temp_json_file.open());
  auto written = QString(temp_json_file.readAll());
  temp_json_file.close();
// different encoding on windows or something
#ifndef _WIN32
  QCOMPARE(written, SONG_TEXT);
#endif
  song_editor.trigger_save();
}

void Tester::test_export() {
  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  song_editor.export_to_file(temp_json_file.fileName());
}

void Tester::test_broken_file_template() {
  QFETCH(const QString, json_song);
  QFETCH(const QString, error_message);
  close_message_later(error_message);
  open_text(json_song);
}

void Tester::test_broken_file_template_data() {
  QTest::addColumn<QString>("json_song");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("parse error")
      << "{"
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing object key - unexpected end of input; "
         "expected string literal";
  QTest::newRow("schema error")
      << "[1]" << "At  of [1] - unexpected instance type\n";
}

void Tester::test_open_empty() {
  open_text(R""""({
    "gain": 5.0,
    "starting_instrument": "Cello",
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_velocity": 64
})"""");
}