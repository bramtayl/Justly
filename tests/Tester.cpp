#include "tests/Tester.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QString>
#include <QTableView>
#include <QTemporaryFile>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <memory>
#include <string>

#include "justly/ChordColumn.hpp"
#include "justly/NoteColumn.hpp"
#include "justly/PercussionColumn.hpp"
#include "justly/justly.hpp"

static const auto BIG_VELOCITY = 126;

static const auto STARTING_KEY_1 = 401.0;
static const auto STARTING_KEY_2 = 402.0;

static const auto STARTING_TEMPO_1 = 150.0;
static const auto STARTING_TEMPO_2 = 100.0;

static const auto STARTING_VELOCITY_1 = 70;
static const auto STARTING_VELOCITY_2 = 80;

static const auto WAIT_TIME = 500;

static const auto OVERLOAD_NUMBER = 15;

static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;

const auto A_MINUS_FREQUENCY = 216.8458;
static const auto A_FREQUENCY = 220.0;
static const auto B_FLAT_FREQUENCY = 233.0819;
static const auto B_FREQUENCY = 246.9417;
static const auto C_FREQUENCY = 261.6256;
static const auto C_SHARP_FREQUENCY = 277.1826;
static const auto D_FREQUENCY = 293.6648;
static const auto E_FLAT_FREQUENCY = 311.1270;
static const auto E_FREQUENCY = 329.6276;
static const auto F_FREQUENCY = 349.2282;
static const auto F_SHARP_FREQUENCY = 369.9944;
static const auto G_FREQUENCY = 391.9954;
static const auto A_FLAT_FREQUENCY = 415.3047;

static const auto *const SONG_TEXT = R""""({
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
            "interval": {
                "numerator": 2
            },
            "notes": [
                {
                    "beats": {
                        "denominator": 2
                    },
                    "instrument": "5th Saw Wave",
                    "interval": {
                        "denominator": 2
                    },
                    "tempo_ratio": {
                        "denominator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "1"
                },
                {
                    "beats": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "instrument": "808 Tom",
                    "interval": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "tempo_ratio": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "velocity_ratio": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "words": "2"
                }
            ],
            "tempo_ratio": {
                "numerator": 2
            },
            "velocity_ratio": {
                "numerator": 2
            },
            "words": "0"
        },
        {
            "beats": {
                "numerator": 2
            },
            "interval": {
                "octave": 1
            },
            "notes": [
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "Acoustic Bass",
                    "interval": {
                        "numerator": 2,
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    },
                    "words": "4"
                },
                {
                    "beats": {
                        "numerator": 2
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "denominator": 2,
                        "octave": 1
                    },
                    "tempo_ratio": {
                        "numerator": 2
                    },
                    "velocity_ratio": {
                        "numerator": 2
                    },
                    "words": "5"
                }
            ],
            "tempo_ratio": {
                "numerator": 2
            },
            "velocity_ratio": {
                "denominator": 2
            },
            "words": "3"
        }
    ],
    "gain": 1.0,
    "starting_key": 220.0,
    "starting_tempo": 100.0,
    "starting_velocity": 10.0
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

void Tester::open_text(const QString &json_song) const {
  QTemporaryFile json_file;
  if (json_file.open()) {
    json_file.write(json_song.toStdString().c_str());
    json_file.close();
  }
  open_file(song_editor_pointer, json_file.fileName());
}

Tester::Tester()
    : song_editor_pointer(make_song_editor()),
      table_view_pointer(get_table_view_pointer(song_editor_pointer)) {}

Tester::~Tester() {
  delete_song_editor(song_editor_pointer);
}

void Tester::initTestCase() {
  QCOMPARE_NE(table_view_pointer, nullptr);
  register_converters();
  open_text(SONG_TEXT);
}

void Tester::test_to_string_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QString, text);

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole).toString(), text);
}

void Tester::test_to_string_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QString>("text");

  QTest::newRow("denominator interval")
      << get_note_index(song_editor_pointer, 0, note_interval_column) << "/2";
  QTest::newRow("numerator octave interval")
      << get_note_index(song_editor_pointer, 0, note_interval_column) << "2o1";
  QTest::newRow("denominator rational")
      << get_note_index(song_editor_pointer, 0, note_beats_column) << "/2";
  QTest::newRow("numerator denominator rational")
      << get_note_index(song_editor_pointer, 1, note_beats_column) << "3/2";
}

void Tester::test_chords_count() const {
  QCOMPARE(table_view_pointer->model()->rowCount(), 4);
}

void Tester::test_notes_count_template() const {
  QFETCH(const int, chord_number);
  QFETCH(const int, row_count);
  trigger_edit_notes(song_editor_pointer, chord_number);
  QCOMPARE(table_view_pointer->model()->rowCount(), row_count);
  undo(song_editor_pointer);
}

void Tester::test_notes_count_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("row_count");

  QTest::newRow("first chord") << 0 << 2;
  QTest::newRow("second chord") << 1 << 2;
  QTest::newRow("third chord") << 2 << 2;
  QTest::newRow("fourth chord") << 3 << 2;
}

void Tester::test_column_count() const {
  QCOMPARE(table_view_pointer->model()->columnCount(), 7);
  trigger_edit_notes(song_editor_pointer, 0);
  QCOMPARE(table_view_pointer->model()->columnCount(), 6);
  undo(song_editor_pointer);
  trigger_edit_percussions(song_editor_pointer, 0);
  QCOMPARE(table_view_pointer->model()->columnCount(), 5);
  undo(song_editor_pointer);
}

void Tester::test_gain_control() const {
  auto old_gain = get_gain(song_editor_pointer);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  set_gain(song_editor_pointer, NEW_GAIN_1);
  QCOMPARE(get_gain(song_editor_pointer), NEW_GAIN_1);
  set_gain(song_editor_pointer, NEW_GAIN_2);
  QCOMPARE(get_gain(song_editor_pointer), NEW_GAIN_2);

  undo(song_editor_pointer);
  QCOMPARE(get_gain(song_editor_pointer), old_gain);
}

void Tester::test_starting_key_control() const {
  auto old_key = get_starting_key(song_editor_pointer);
  QCOMPARE_NE(old_key, STARTING_KEY_1);
  QCOMPARE_NE(old_key, STARTING_KEY_2);

  // test combining
  set_starting_key(song_editor_pointer, STARTING_KEY_1);
  QCOMPARE(get_starting_key(song_editor_pointer), STARTING_KEY_1);
  set_starting_key(song_editor_pointer, STARTING_KEY_2);
  QCOMPARE(get_starting_key(song_editor_pointer), STARTING_KEY_2);
  undo(song_editor_pointer);
  QCOMPARE(get_starting_key(song_editor_pointer), old_key);
}

void Tester::test_starting_velocity_control() const {
  auto old_velocity = get_starting_velocity(song_editor_pointer);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_1);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_2);

  // test combining
  set_starting_velocity(song_editor_pointer, STARTING_VELOCITY_1);
  QCOMPARE(get_starting_velocity(song_editor_pointer), STARTING_VELOCITY_1);
  set_starting_velocity(song_editor_pointer, STARTING_VELOCITY_2);
  QCOMPARE(get_starting_velocity(song_editor_pointer), STARTING_VELOCITY_2);
  undo(song_editor_pointer);
  QCOMPARE(get_starting_velocity(song_editor_pointer), old_velocity);
}

void Tester::test_starting_tempo_control() const {
  auto old_tempo = get_starting_tempo(song_editor_pointer);

  // test combining
  set_starting_tempo(song_editor_pointer, STARTING_TEMPO_1);
  QCOMPARE(get_starting_tempo(song_editor_pointer), STARTING_TEMPO_1);
  set_starting_tempo(song_editor_pointer, STARTING_TEMPO_2);
  QCOMPARE(get_starting_tempo(song_editor_pointer), STARTING_TEMPO_2);
  undo(song_editor_pointer);
  QCOMPARE(get_starting_tempo(song_editor_pointer), old_tempo);
}

void Tester::test_chord_column_headers_template() const {
  QFETCH(const int, position);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);

  QCOMPARE(table_view_pointer->model()->headerData(position, orientation, role), value);
}

void Tester::test_chord_column_headers_template_data() {
  QTest::addColumn<int>("position");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("interval") << chord_interval_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Interval");
  QTest::newRow("beats") << chord_beats_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Beats");
  QTest::newRow("velocity") << chord_velocity_ratio_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Velocity ratio");
  QTest::newRow("tempo") << chord_tempo_ratio_column << Qt::Horizontal
                         << Qt::DisplayRole << QVariant("Tempo ratio");
  QTest::newRow("words") << chord_words_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Words");
  QTest::newRow("notes") << chord_notes_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Notes");
  QTest::newRow("percussions") << chord_notes_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Percussions");
  QTest::newRow("rows")
      << 0 << Qt::Vertical << Qt::DisplayRole << QVariant(1);
  QTest::newRow("wrong role")
      << chord_interval_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
}


void Tester::test_note_column_headers_template() const {
  QFETCH(const int, position);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);

  trigger_edit_notes(song_editor_pointer, 0);
  QCOMPARE(table_view_pointer->model()->headerData(position, orientation, role), value);
  undo(song_editor_pointer);
}

void Tester::test_note_column_headers_template_data() {
  QTest::addColumn<int>("position");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("instrument") << note_instrument_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Instrument");
  QTest::newRow("interval") << note_interval_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Interval");
  QTest::newRow("beats") << note_beats_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Beats");
  QTest::newRow("velocity") << note_velocity_ratio_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Velocity ratio");
  QTest::newRow("tempo") << note_tempo_ratio_column << Qt::Horizontal
                         << Qt::DisplayRole << QVariant("Tempo ratio");
  QTest::newRow("words") << note_words_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Words");
  QTest::newRow("rows")
      << 0 << Qt::Vertical << Qt::DisplayRole << QVariant(1);
  QTest::newRow("wrong role")
      << note_instrument_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
}


void Tester::test_percussion_column_headers_template() const {
  QFETCH(const int, position);
  QFETCH(const Qt::Orientation, orientation);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);

  trigger_edit_percussions(song_editor_pointer, 0);
  QCOMPARE(table_view_pointer->model()->headerData(position, orientation, role), value);
  undo(song_editor_pointer);
}

void Tester::test_percussion_column_headers_template_data() {
  QTest::addColumn<int>("position");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("set") << percussion_set_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Set");
  QTest::newRow("instrument") << percussion_instrument_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Instrument");
  QTest::newRow("beats") << percussion_beats_column << Qt::Horizontal
                            << Qt::DisplayRole << QVariant("Beats");
  QTest::newRow("velocity") << percussion_velocity_ratio_column << Qt::Horizontal
                         << Qt::DisplayRole << QVariant("Velocity ratio");
  QTest::newRow("tempo") << percussion_tempo_ratio_column << Qt::Horizontal << Qt::DisplayRole
                         << QVariant("Tempo ratio");
  QTest::newRow("rows")
      << 0 << Qt::Vertical << Qt::DisplayRole << QVariant(1);
  QTest::newRow("wrong role")
      << percussion_set_column << Qt::Horizontal << Qt::DecorationRole << QVariant();
}

static auto get_flags(const QTableView* table_view_pointer, int column) {
  const auto* model_pointer = table_view_pointer->model();
  return model_pointer->flags(model_pointer->index(0, column));
}

void Tester::test_flags() {
  trigger_edit_notes(song_editor_pointer, 0);
  QCOMPARE(get_flags(table_view_pointer, note_interval_column), Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  undo(song_editor_pointer);

  trigger_edit_percussions(song_editor_pointer, 0);
  QCOMPARE(get_flags(table_view_pointer, percussion_set_column), Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  undo(song_editor_pointer);
}

void Tester::test_chord_flags_template() const {
  QFETCH(const int, column);
  QFETCH(const Qt::ItemFlags, item_flags);
  QCOMPARE(get_flags(table_view_pointer, column), item_flags);
}

void Tester::test_chord_flags_template_data() const {
  QTest::addColumn<int>("column");
  QTest::addColumn<Qt::ItemFlags>("item_flags");

  QTest::newRow("interval")
      << chord_interval_column
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  QTest::newRow("notes")
      << chord_notes_column
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("percussions")
      << chord_percussions_column
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void Tester::test_status_template() const {
  QFETCH(const double, frequency);
  QFETCH(const QString, text);

  const auto* model_pointer = table_view_pointer->model();

  set_starting_key(song_editor_pointer, frequency);
  QCOMPARE(model_pointer->data(model_pointer->index(0, chord_interval_column),
                                      Qt::StatusTipRole),
           text);
  undo(song_editor_pointer);
}

void Tester::test_status_template_data() {
  QTest::addColumn<double>("frequency");
  QTest::addColumn<QString>("text");

  QTest::newRow("A minus") << A_MINUS_FREQUENCY << "216.8 Hz; A3 − 25 cents";
  QTest::newRow("A") << A_FREQUENCY << "220 Hz; A3 + 0 cents";
  QTest::newRow("Bb") << B_FLAT_FREQUENCY << "233.1 Hz; B♭3 + 0 cents";
  QTest::newRow("B") << B_FREQUENCY << "246.9 Hz; B3 + 0 cents";
  QTest::newRow("C") << C_FREQUENCY << "261.6 Hz; C4 + 0 cents";
  QTest::newRow("C#") << C_SHARP_FREQUENCY << "277.2 Hz; C♯4 + 0 cents";
  QTest::newRow("D") << D_FREQUENCY << "293.7 Hz; D4 + 0 cents";
  QTest::newRow("Eb") << E_FLAT_FREQUENCY << "311.1 Hz; E♭4 + 0 cents";
  QTest::newRow("E") << E_FREQUENCY << "329.6 Hz; E4 + 0 cents";
  QTest::newRow("F") << F_FREQUENCY << "349.2 Hz; F4 + 0 cents";
  QTest::newRow("F#") << F_SHARP_FREQUENCY << "370 Hz; F♯4 + 0 cents";
  QTest::newRow("G") << G_FREQUENCY << "392 Hz; G4 + 0 cents";
  QTest::newRow("Ab") << A_FLAT_FREQUENCY << "415.3 Hz; A♭4 + 0 cents";
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
      << get_chord_index(song_editor_pointer, 0, chord_interval_column) << Qt::DisplayRole << QVariant("Chord");
  QTest::newRow("first chord decoration")
      << get_chord_index(song_editor_pointer, 0, chord_interval_column) << Qt::DecorationRole << QVariant();
  QTest::newRow("first note symbol")
      << get_note_index(song_editor_pointer, 0, note_interval_column) << Qt::DisplayRole << QVariant("Note");
  QTest::newRow("first note decoration")
      << get_note_index(song_editor_pointer, 0, note_interval_column) << Qt::DecorationRole << QVariant();
}

void Tester::test_background() const {
  QCOMPARE_NE(chords_model_pointer->data(get_chord_index(song_editor_pointer, 0, chord_interval_column),
                                         Qt::BackgroundRole),
              chords_model_pointer->data(get_note_index(song_editor_pointer, 0, note_interval_column),
                                         Qt::BackgroundRole));
}

void Tester::test_delegate_template() const {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_value = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);
  QCOMPARE_NE(copy_value, paste_value);

  auto *cell_editor_pointer = create_editor(song_editor_pointer, paste_index);

  QCOMPARE(cell_editor_pointer->property(
               cell_editor_pointer->metaObject()->userProperty().name()),
           paste_value);

  set_editor(song_editor_pointer, cell_editor_pointer, paste_index, copy_value);

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), copy_value);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
}

void Tester::test_delegate_template_data() const {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("instrument")
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 1, note_instrument_column);
  QTest::newRow("interval") << get_chord_index(song_editor_pointer, 0, chord_interval_column)
                            << get_chord_index(song_editor_pointer, 2, chord_interval_column);
  QTest::newRow("beats") << get_chord_index(song_editor_pointer, 0, chord_beats_column)
                         << get_chord_index(song_editor_pointer, 2, chord_beats_column);
  QTest::newRow("velocity ratio")
      << get_chord_index(song_editor_pointer, 0, chord_velocity_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_velocity_ratio_column);
  QTest::newRow("tempo ratio")
      << get_chord_index(song_editor_pointer, 0, chord_tempo_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_tempo_ratio_column);
  QTest::newRow("words") << get_chord_index(song_editor_pointer, 0, chord_words_column)
                         << get_chord_index(song_editor_pointer, 2, chord_words_column);
}

void Tester::test_no_set_value() const {
  // setData only works for the edit role
  QVERIFY(!(chords_model_pointer->setData(get_chord_index(song_editor_pointer, 0, chord_interval_column),
                                          QVariant(), Qt::DecorationRole)));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_value = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);
  QCOMPARE_NE(copy_value, paste_value);

  QVERIFY(chords_model_pointer->setData(paste_index, copy_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::DisplayRole),
           copy_value);
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), copy_value);

  undo(song_editor_pointer);

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::DisplayRole),
           paste_value);
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << get_chord_index(song_editor_pointer, 2, chord_interval_column);
  QTest::newRow("chord beats") << get_chord_index(song_editor_pointer, 0, chord_beats_column)
                               << get_chord_index(song_editor_pointer, 2, chord_beats_column);
  QTest::newRow("chord velocity ratio")
      << get_chord_index(song_editor_pointer, 0, chord_velocity_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_velocity_ratio_column);
  QTest::newRow("chord tempo ratio")
      << get_chord_index(song_editor_pointer, 0, chord_tempo_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_tempo_ratio_column);
  QTest::newRow("chord words") << get_chord_index(song_editor_pointer, 0, chord_words_column)
                               << get_chord_index(song_editor_pointer, 2, chord_words_column);
  QTest::newRow("note instrument")
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 0, note_instrument_column);
  QTest::newRow("note interval")
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column);
  QTest::newRow("note beats") << get_note_index(song_editor_pointer, 0, note_beats_column)
                              << get_note_index(song_editor_pointer, 0, note_beats_column);
  QTest::newRow("note velocity")
      << get_note_index(song_editor_pointer, 0, note_velocity_ratio_column)
      << get_note_index(song_editor_pointer, 0, note_velocity_ratio_column);
  QTest::newRow("note tempo")
      << get_note_index(song_editor_pointer, 0, note_tempo_ratio_column)
      << get_note_index(song_editor_pointer, 0, note_tempo_ratio_column);
  QTest::newRow("note words") << get_note_index(song_editor_pointer, 0, note_words_column)
                              << get_note_index(song_editor_pointer, 0, note_words_column);
}

void Tester::test_delete_cell_template() const {
  QFETCH(const QModelIndex, index);

  auto old_value = chords_model_pointer->data(index, Qt::EditRole);
  QVERIFY(!old_value.toString().isEmpty());

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_delete(song_editor_pointer);
  clear_selection();

  QVERIFY(chords_model_pointer->data(index, Qt::EditRole).toString().isEmpty());
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
};

void Tester::test_delete_cell_template_data() const {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("third chord interval")
      << get_chord_index(song_editor_pointer, 2, chord_interval_column);

  QTest::newRow("third chord beats")
      << get_chord_index(song_editor_pointer, 2, chord_beats_column);

  QTest::newRow("third chord words")
      << get_chord_index(song_editor_pointer, 2, chord_words_column);

  QTest::newRow("third note instrument")
      << get_note_index(song_editor_pointer, 0, note_instrument_column);
};

void Tester::test_delete_3_groups_template() const {
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
  trigger_delete(song_editor_pointer);
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
  undo(song_editor_pointer);

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

void Tester::test_delete_3_groups_template_data() const {
  QTest::addColumn<QModelIndex>("top_left_index_1");
  QTest::addColumn<QModelIndex>("bottom_right_index_1");
  QTest::addColumn<QModelIndex>("top_left_index_2");
  QTest::addColumn<QModelIndex>("bottom_right_index_2");
  QTest::addColumn<QModelIndex>("top_left_index_3");
  QTest::addColumn<QModelIndex>("bottom_right_index_3");

  QTest::newRow("note chord note")
      << get_note_index(song_editor_pointer, 1, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 3, chord_interval_column)
      << get_chord_index(song_editor_pointer, 3, chord_beats_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 0, note_beats_column);

  QTest::newRow("chord notes chord")
      << get_chord_index(song_editor_pointer, 2, chord_interval_column)
      << get_chord_index(song_editor_pointer, 2, chord_beats_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 3, chord_interval_column)
      << get_chord_index(song_editor_pointer, 3, chord_beats_column);
}

void Tester::test_paste_cell_template() const {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_value = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);

  QCOMPARE_NE(copy_value, paste_value);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection();

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), copy_value);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
}

void Tester::test_paste_cell_template_data() const {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << get_chord_index(song_editor_pointer, 2, chord_interval_column);

  QTest::newRow("chord beats") << get_chord_index(song_editor_pointer, 0, chord_beats_column)
                               << get_chord_index(song_editor_pointer, 2, chord_beats_column);

  QTest::newRow("chord tempo ratio")
      << get_chord_index(song_editor_pointer, 0, chord_tempo_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_tempo_ratio_column);

  QTest::newRow("chord velocity ratio")
      << get_chord_index(song_editor_pointer, 0, chord_velocity_ratio_column)
      << get_chord_index(song_editor_pointer, 2, chord_velocity_ratio_column);

  QTest::newRow("chord words") << get_chord_index(song_editor_pointer, 0, chord_words_column)
                               << get_chord_index(song_editor_pointer, 2, chord_words_column);

  QTest::newRow("note interval")
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column);

  QTest::newRow("note beats") << get_note_index(song_editor_pointer, 0, note_beats_column)
                              << get_note_index(song_editor_pointer, 0, note_beats_column);

  QTest::newRow("note tempo ratio")
      << get_note_index(song_editor_pointer, 0, note_tempo_ratio_column)
      << get_note_index(song_editor_pointer, 0, note_tempo_ratio_column);

  QTest::newRow("note velocity ratio")
      << get_note_index(song_editor_pointer, 0, note_velocity_ratio_column)
      << get_note_index(song_editor_pointer, 0, note_velocity_ratio_column);

  QTest::newRow("note words") << get_note_index(song_editor_pointer, 0, note_words_column)
                              << get_note_index(song_editor_pointer, 0, note_words_column);

  QTest::newRow("note instrument")
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 0, note_instrument_column);
}

void Tester::test_cut_paste_cell_template() const {
  QFETCH(const QModelIndex, cut_index);
  QFETCH(const QModelIndex, paste_index);

  const auto cut_value = chords_model_pointer->data(cut_index);
  const auto paste_value = chords_model_pointer->data(paste_index);

  QVERIFY(!cut_value.toString().isEmpty());
  QCOMPARE_NE(cut_value, paste_value);

  selector_pointer->select(cut_index, QItemSelectionModel::Select);
  trigger_cut(song_editor_pointer);
  clear_selection();

  QVERIFY(chords_model_pointer->data(cut_index).toString().isEmpty());

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), cut_value);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->data(cut_index, Qt::EditRole), cut_value);
}

void Tester::test_cut_paste_cell_template_data() const {
  QTest::addColumn<QModelIndex>("cut_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << get_chord_index(song_editor_pointer, 2, chord_interval_column)
      << get_chord_index(song_editor_pointer, 0, chord_interval_column);
}

void Tester::test_paste_3_groups_template() const {
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
  trigger_copy(song_editor_pointer);
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
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           copy_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_3, Qt::EditRole),
           copy_bottom_right_data);
  undo(song_editor_pointer);

  QCOMPARE(chords_model_pointer->data(paste_top_left_index_1, Qt::EditRole),
           paste_top_left_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_right_index_3, Qt::EditRole),
           paste_bottom_right_data);
}

void Tester::test_paste_3_groups_template_data() const {
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
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << get_chord_index(song_editor_pointer, 0, chord_beats_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 1, chord_interval_column)
      << get_chord_index(song_editor_pointer, 1, chord_beats_column)
      << get_note_index(song_editor_pointer, 1, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 3, chord_interval_column)
      << get_chord_index(song_editor_pointer, 3, chord_beats_column)
      << get_note_index(song_editor_pointer, 1, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column);

  QTest::newRow("note then chord then note")
      << get_note_index(song_editor_pointer, 1, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 1, chord_interval_column)
      << get_chord_index(song_editor_pointer, 1, chord_beats_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 2, chord_interval_column)
      << get_chord_index(song_editor_pointer, 2, chord_beats_column)
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << get_note_index(song_editor_pointer, 1, note_beats_column)
      << get_chord_index(song_editor_pointer, 3, chord_interval_column)
      << get_chord_index(song_editor_pointer, 3, chord_beats_column);
}

void Tester::test_paste_truncate_template() const {
  QFETCH(const QModelIndex, copy_top_index);
  QFETCH(const QModelIndex, copy_bottom_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_top_data = chords_model_pointer->data(copy_top_index, Qt::EditRole);
  auto paste_value = chords_model_pointer->data(paste_index, Qt::EditRole);

  selector_pointer->select(QItemSelection(copy_top_index, copy_bottom_index),
                           QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection();

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole),
           copy_top_data);
  undo(song_editor_pointer);

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_value);
};

void Tester::test_paste_truncate_template_data() const {
  QTest::addColumn<QModelIndex>("copy_top_index");
  QTest::addColumn<QModelIndex>("copy_bottom_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("first notes")
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 1, note_instrument_column)
      << get_note_index(song_editor_pointer, 1, note_instrument_column);
};

void Tester::test_paste_recycle_template() const {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_top_index);
  QFETCH(const QModelIndex, paste_bottom_index);

  auto copy_data = chords_model_pointer->data(copy_index, Qt::EditRole);
  auto paste_top_data =
      chords_model_pointer->data(paste_top_index, Qt::EditRole);
  auto paste_bottom_data =
      chords_model_pointer->data(paste_bottom_index, Qt::EditRole);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection();

  selector_pointer->select(QItemSelection(paste_top_index, paste_bottom_index),
                           QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_top_index, Qt::EditRole),
           copy_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_index, Qt::EditRole),
           copy_data);
  undo(song_editor_pointer);

  QCOMPARE(chords_model_pointer->data(paste_top_index, Qt::EditRole),
           paste_top_data);
  QCOMPARE(chords_model_pointer->data(paste_bottom_index, Qt::EditRole),
           paste_bottom_data);
};

void Tester::test_paste_recycle_template_data() const {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_top_index");
  QTest::addColumn<QModelIndex>("paste_bottom_index");

  QTest::newRow("first two notes")
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << get_note_index(song_editor_pointer, 1, note_instrument_column);
};

void Tester::test_insert_into() const {
  auto chord_index = get_chord_index(song_editor_pointer, 0, chord_interval_column);
  auto old_row_count = chords_model_pointer->rowCount(chord_index);
  selector_pointer->select(chord_index, QItemSelectionModel::Select);
  trigger_insert_into(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(chord_index), old_row_count + 1);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->rowCount(chord_index), old_row_count);
}

void Tester::test_new_chord_from_chord() const {
  auto chord_beats_value =
      chords_model_pointer->data(get_chord_index(song_editor_pointer, 2, chord_beats_column));
  QVERIFY(!chord_beats_value.toString().isEmpty());

  selector_pointer->select(get_chord_index(song_editor_pointer, 2, chord_interval_column),
                           QItemSelectionModel::Select);
  trigger_insert_after(song_editor_pointer);
  clear_selection();
  QCOMPARE(chords_model_pointer->data(
               get_chord_index(song_editor_pointer, 3, chord_beats_column), Qt::EditRole),
           chord_beats_value);
  undo(song_editor_pointer);
}

void Tester::test_new_note_from_chord() const {
  auto chord_beats_value =
      chords_model_pointer->data(get_chord_index(song_editor_pointer, 2, chord_beats_column));
  QVERIFY(!chord_beats_value.toString().isEmpty());

  auto chord_words_value =
      chords_model_pointer->data(get_chord_index(song_editor_pointer, 2, chord_words_column));
  QVERIFY(!chord_words_value.toString().isEmpty());

  selector_pointer->select(get_chord_index(song_editor_pointer, 2, chord_interval_column),
                           QItemSelectionModel::Select);
  trigger_insert_into(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               get_note_index(song_editor_pointer, 0, note_beats_column), Qt::EditRole),
           chord_beats_value);
  QCOMPARE(chords_model_pointer->data(
               get_note_index(song_editor_pointer, 0, note_words_column), Qt::EditRole),
           chord_words_value);
  undo(song_editor_pointer);
}

void Tester::test_new_note_from_note() const {
  auto note_beats_value = chords_model_pointer->data(
      get_note_index(song_editor_pointer, 0, note_beats_column));
  QVERIFY(!note_beats_value.toString().isEmpty());

  auto note_velocity_ratio_value = chords_model_pointer->data(
      get_note_index(song_editor_pointer, 0, note_velocity_ratio_column));
  QVERIFY(!note_velocity_ratio_value.toString().isEmpty());

  auto note_tempo_ratio_value = chords_model_pointer->data(
      get_note_index(song_editor_pointer, 0, note_tempo_ratio_column));
  QVERIFY(!note_tempo_ratio_value.toString().isEmpty());

  auto note_words_value = chords_model_pointer->data(
      get_note_index(song_editor_pointer, 0, note_words_column));
  QVERIFY(!note_words_value.toString().isEmpty());

  selector_pointer->select(get_note_index(song_editor_pointer, 0, note_interval_column),
                           QItemSelectionModel::Select);
  trigger_insert_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               get_note_index(song_editor_pointer, 1, note_beats_column), Qt::EditRole),
           note_beats_value);
  QCOMPARE(chords_model_pointer->data(
               get_note_index(song_editor_pointer, 1, note_velocity_ratio_column),
               Qt::EditRole),
           note_velocity_ratio_value);
  QCOMPARE(
      chords_model_pointer->data(
          get_note_index(song_editor_pointer, 1, note_tempo_ratio_column), Qt::EditRole),
      note_tempo_ratio_value);
  QCOMPARE(chords_model_pointer->data(
               get_note_index(song_editor_pointer, 1, note_words_column), Qt::EditRole),
           note_words_value);
  undo(song_editor_pointer);
}

void Tester::test_insert_rows_template() const {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_insert_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_insert_rows_template_data() const {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("insert chord after") << get_chord_index(song_editor_pointer, 0, chord_interval_column);
  QTest::newRow("insert note after") << get_note_index(song_editor_pointer, 0, note_interval_column);
}

void Tester::test_delete_rows_template() const {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_delete(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count - 1);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_delete_rows_template_data() const {
  QTest::addColumn<QModelIndex>("index");

  QTest::newRow("delete chord") << get_chord_index(song_editor_pointer, 0, chord_interval_column);
  QTest::newRow("delete note") << get_note_index(song_editor_pointer, 0, note_interval_column);
}

void Tester::test_paste_rows_template() const {
  QFETCH(const QModelIndex, index);

  auto parent_index = chords_model_pointer->parent(index);
  auto old_row_count = chords_model_pointer->rowCount(parent_index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection();

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  undo(song_editor_pointer);
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_paste_rows_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("parent_row_count");

  QTest::newRow("paste chord after") << get_chord_index(song_editor_pointer, 0, chord_interval_column);
  QTest::newRow("paste note after") << get_note_index(song_editor_pointer, 0, note_interval_column);
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
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();
}

void Tester::test_bad_paste_template_data() const {
  QTest::addColumn<QString>("copied");
  QTest::addColumn<QString>("mime_type");
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("unparsable chord")
      << "[" << "application/prs.chords+json" << get_chord_index(song_editor_pointer, 0, chord_interval_column)

      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("empty chords")
      << "[]" << "application/prs.chords+json" << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "Nothing to paste!";

  QTest::newRow("wrong type chord")
      << "{\"a\": 1}" << "application/prs.chords+json"
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("unparsable note")
      << "[" << "application/prs.notes+json" << get_note_index(song_editor_pointer, 0, note_interval_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong type note")
      << "{\"a\": 1}" << "application/prs.notes+json"
      << get_note_index(song_editor_pointer, 0, note_interval_column)
      << "At  of {\"a\":1} - unexpected instance type\n";

  QTest::newRow("wrong row mime type")
      << "{\"a\": 1}" << "not a mime" << get_chord_index(song_editor_pointer, 0, chord_interval_column)

      << "Cannot paste not a mime into destination needing chords";

  QTest::newRow("wrong cell mime type")
      << "{\"a\": 1}" << "not a mime"
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "Cannot paste not a mime into destination needing cells";

  QTest::newRow("unparsable interval")
      << "[" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable rational")
      << "[" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_beats_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable instrument")
      << "[" << "application/prs.cells+json"
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("unparsable octave")
      << "[" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_words_column)

      << "[json.exception.parse_error.101] parse error at line 1, column 2: "
         "syntax error while parsing value - unexpected end of input; expected "
         "'[', '{', or a literal";

  QTest::newRow("wrong interval type")
      << "[1]" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong rational type")
      << "[1]" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_beats_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong instrument type")
      << "[1]" << "application/prs.cells+json"
      << get_note_index(song_editor_pointer, 0, note_instrument_column)
      << "At  of [1] - unexpected instance type\n";

  QTest::newRow("wrong words type")
      << "[1]" << "application/prs.cells+json"
      << get_chord_index(song_editor_pointer, 0, chord_words_column)
      << "At  of [1] - unexpected instance type\n";
}

void Tester::test_paste_wrong_level_template() {
  QFETCH(const QModelIndex, copy_index);
  QFETCH(const QModelIndex, paste_index);
  QFETCH(const QString, error_message);

  selector_pointer->select(copy_index, QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection();

  close_message_later(error_message);

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  trigger_paste_cells_or_after(song_editor_pointer);
  clear_selection();
}

void Tester::test_paste_wrong_level_template_data() const {
  QTest::addColumn<QModelIndex>("copy_index");
  QTest::addColumn<QModelIndex>("paste_index");
  QTest::addColumn<QString>("error_message");

  QTest::newRow("chords to notes")
      << get_chord_index(song_editor_pointer, 0, chord_interval_column) << get_note_index(song_editor_pointer, 0, note_interval_column)
      << "Cannot paste chords into destination needing notes";

  QTest::newRow("notes to chords")
      << get_note_index(song_editor_pointer, 0, note_interval_column) << get_chord_index(song_editor_pointer, 0, chord_interval_column)
      << "Cannot paste notes into destination needing chords";
}

void Tester::test_too_loud() {
  set_starting_velocity(song_editor_pointer, BIG_VELOCITY);

  close_message_later(
      "Velocity exceeds 127 for chord 3, note 2. Playing with velocity 127.");

  selector_pointer->select(get_note_index(song_editor_pointer, 1, note_interval_column),
                           QItemSelectionModel::Select);
  trigger_play(song_editor_pointer);
  clear_selection();

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor_pointer);
  undo(song_editor_pointer);
}

void Tester::test_too_many_channels() {
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    selector_pointer->select(get_chord_index(song_editor_pointer, 0, chord_interval_column),
                             QItemSelectionModel::Select);
    trigger_insert_into(song_editor_pointer);
    clear_selection();
  }

  close_message_later(
      "Out of MIDI channels for chord 1, note 17. Not playing note.");

  selector_pointer->select(get_chord_index(song_editor_pointer, 0, chord_interval_column),
                           QItemSelectionModel::Select);
  trigger_play(song_editor_pointer);
  clear_selection();

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor_pointer);

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    undo(song_editor_pointer);
  }
}

void Tester::test_play_template() const {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  selector_pointer->select(QItemSelection(first_index, last_index),
                           QItemSelectionModel::Select);
  trigger_play(song_editor_pointer);
  // first cut off early
  trigger_play(song_editor_pointer);
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  clear_selection();
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << get_chord_index(song_editor_pointer, 0, chord_interval_column) << get_chord_index(song_editor_pointer, 1, chord_interval_column);
  QTest::newRow("second chord")
      << get_chord_index(song_editor_pointer, 1, chord_interval_column) << get_chord_index(song_editor_pointer, 1, chord_interval_column);

  auto first_chord_second_note_index = get_note_index(song_editor_pointer, 1, note_interval_column);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index = get_note_index(song_editor_pointer, 0, note_interval_column);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}

void Tester::test_save() const {
  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  auto file_name = temp_json_file.fileName();
  save_as_file(song_editor_pointer, file_name);

  QCOMPARE(get_current_file(song_editor_pointer), file_name);

  QVERIFY(temp_json_file.open());
  auto written = QString(temp_json_file.readAll());
  temp_json_file.close();
// different encoding on windows or something
#ifndef _WIN32
  QCOMPARE(written, SONG_TEXT);
#endif
  trigger_save(song_editor_pointer);
}

void Tester::test_export() const {
  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  export_to_file(song_editor_pointer, temp_json_file.fileName());
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

void Tester::test_open_empty() const {
  open_text(R""""({
    "gain": 5.0,
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_velocity": 64
})"""");
}