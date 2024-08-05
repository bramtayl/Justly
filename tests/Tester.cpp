#include "tests/Tester.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QByteArray>
#include <QClipboard>
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

void copy_text(const std::string &text, const QString &mime_type) {
  auto *new_data_pointer = std::make_unique<QMimeData>().release();

  Q_ASSERT(new_data_pointer != nullptr);
  new_data_pointer->setData(mime_type, text.c_str());

  auto *clipboard_pointer = QGuiApplication::clipboard();
  Q_ASSERT(clipboard_pointer != nullptr);
  clipboard_pointer->setMimeData(new_data_pointer);
}

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

Tester::Tester() : 
    song_editor({}),
    chords_view_pointer(song_editor.get_chords_view_pointer()),
    selector_pointer(chords_view_pointer->selectionModel()),
    chords_model_pointer(chords_view_pointer->model()) {
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
  QTest::newRow("first chord") << song_editor.get_chord_index(0) << 2;
  QTest::newRow("second chord") << song_editor.get_chord_index(1) << 1;
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

void Tester::test_playback_volume_control() {
  auto old_playback_volume = song_editor.get_playback_volume();
  song_editor.set_playback_volume_control(NEW_VOLUME_PERCENT);
  QCOMPARE(song_editor.get_playback_volume(),
           NEW_VOLUME_PERCENT / PERCENT * MAX_GAIN);
  song_editor.set_playback_volume_control(
      static_cast<int>(old_playback_volume * PERCENT / MAX_GAIN));
}

void Tester::test_starting_instrument_control() const {
  const auto *original_value = get_instrument_pointer("Marimba");
  const auto *new_value = get_instrument_pointer("Oboe");
  const auto *new_value_2 = get_instrument_pointer("Ocarina");

  const auto *old_value = song_editor.get_instrument();
  QCOMPARE(old_value, get_instrument_pointer("Marimba"));

  // test change
  song_editor.set_instrument(new_value);
  QCOMPARE(song_editor.get_instrument(), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_instrument(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_instrument(), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_instrument(), original_value);

  // test combining
  song_editor.set_instrument(new_value);
  song_editor.set_instrument(new_value_2);
  QCOMPARE(song_editor.get_instrument(), new_value_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_instrument(), original_value);
}

void Tester::test_starting_key_control() const {
  auto old_value = song_editor.get_key();
  QCOMPARE(old_value, ORIGINAL_KEY);

  // test change
  song_editor.set_key(STARTING_KEY_1);
  QCOMPARE(song_editor.get_key(), STARTING_KEY_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_key(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_key(), STARTING_KEY_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_key(), ORIGINAL_KEY);

  // test combining
  song_editor.set_key(STARTING_KEY_1);
  song_editor.set_key(STARTING_KEY_2);
  QCOMPARE(song_editor.get_key(), STARTING_KEY_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_key(), ORIGINAL_KEY);
}

void Tester::test_starting_volume_control() const {
  auto old_value = song_editor.get_volume_percent();
  QCOMPARE(old_value, ORIGINAL_VOLUME);

  // test change
  song_editor.set_volume_percent(STARTING_VOLUME_1);
  QCOMPARE(song_editor.get_volume_percent(), STARTING_VOLUME_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_volume_percent(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_volume_percent(), STARTING_VOLUME_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_volume_percent(), ORIGINAL_VOLUME);

  // test combining
  song_editor.set_volume_percent(STARTING_VOLUME_1);
  song_editor.set_volume_percent(STARTING_VOLUME_2);
  QCOMPARE(song_editor.get_volume_percent(), STARTING_VOLUME_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_volume_percent(), ORIGINAL_VOLUME);
}

void Tester::test_starting_tempo_control() const {
  auto old_value = song_editor.get_tempo();
  QCOMPARE(old_value, ORIGINAL_TEMPO);

  // test change
  song_editor.set_tempo(STARTING_TEMPO_1);
  QCOMPARE(song_editor.get_tempo(), STARTING_TEMPO_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_tempo(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_tempo(), STARTING_TEMPO_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_tempo(), ORIGINAL_TEMPO);

  // test combining
  song_editor.set_tempo(STARTING_TEMPO_1);
  song_editor.set_tempo(STARTING_TEMPO_2);
  QCOMPARE(song_editor.get_tempo(), STARTING_TEMPO_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_tempo(), ORIGINAL_TEMPO);
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

  QTest::newRow("select_chord_then_note")
      << song_editor.get_chord_index(0) << song_editor.get_note_index(0, 0);
  QTest::newRow("select_note_then_chord")
      << song_editor.get_note_index(0, 0) << song_editor.get_chord_index(0);
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
      << song_editor.get_chord_index(0)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first_chord_interval_flag")
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
  QTest::newRow("first_chord_symbol")
      << song_editor.get_chord_index(0) << Qt::DisplayRole << QVariant("♫");
  QTest::newRow("first_chord_decoration")
      << song_editor.get_chord_index(0) << Qt::DecorationRole << QVariant();
  QTest::newRow("first_note_symbol")
      << song_editor.get_note_index(0, 0) << Qt::DisplayRole << QVariant("♪");
  QTest::newRow("first_note_decoration")
      << song_editor.get_note_index(0, 0) << Qt::DecorationRole << QVariant();
  QTest::newRow("second_note_interval")
      << song_editor.get_note_index(0, 1, interval_column) << Qt::DisplayRole
      << QVariant::fromValue(Interval(2, 2, 1));
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
      << song_editor.get_chord_index(0) << song_editor.get_note_index(0, 0)
      << Qt::BackgroundRole;
}

void Tester::test_delegate_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

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
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("instrument editor")
      << song_editor.get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval editor")
      << song_editor.get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats editor")
      << song_editor.get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("volume editor")
      << song_editor.get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("tempo editor")
      << song_editor.get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("words editor") << song_editor.get_chord_index(0, words_column)
                                << QVariant("") << QVariant("hello");
}

void Tester::test_set_value() const {
  // setData only works for the edit role
  QVERIFY(!(chords_model_pointer->setData(song_editor.get_chord_index(0),
                                          QVariant(), Qt::DecorationRole)));
}

void Tester::test_set_value_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  QVERIFY(chords_model_pointer->setData(index, new_value, Qt::EditRole));

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), new_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), new_value);

  song_editor.undo();
  song_editor.redo();
  song_editor.undo();

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
  QCOMPARE(chords_model_pointer->data(index, Qt::DisplayRole), old_value);
}

void Tester::test_set_value_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("first_chord_interval")
      << song_editor.get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant::fromValue(Interval(2));
  QTest::newRow("first_chord_beats")
      << song_editor.get_chord_index(0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_volume")
      << song_editor.get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_tempo")
      << song_editor.get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_chord_words")
      << song_editor.get_chord_index(0, words_column) << QVariant("")
      << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << song_editor.get_chord_index(0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("first_note_interval")
      << song_editor.get_note_index(0, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant::fromValue(Interval(2));
  QTest::newRow("first_note_beats")
      << song_editor.get_note_index(0, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_volume")
      << song_editor.get_note_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_tempo")
      << song_editor.get_note_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));
  QTest::newRow("first_note_words")
      << song_editor.get_note_index(0, 0, words_column) << QVariant("")
      << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << song_editor.get_note_index(0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
}

void Tester::test_delete_cell_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, empty_value);
  QFETCH(const QVariant, old_value);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_delete();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), empty_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(index, Qt::EditRole), old_value);
};

void Tester::test_delete_cell_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("empty_value");
  QTest::addColumn<QVariant>("old_value");

  QTest::newRow("second chord interval")
      << song_editor.get_chord_index(1, interval_column)
      << QVariant::fromValue(Interval())
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("second chord beats")
      << song_editor.get_chord_index(1, beats_column)
      << QVariant::fromValue(Rational()) << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("second chord words")
      << song_editor.get_chord_index(1, words_column) << QVariant("")
      << QVariant("hello");

  QTest::newRow("second chord instrument")
      << song_editor.get_chord_index(1, instrument_column)
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
  song_editor.trigger_delete();
  clear_selection();

  QCOMPARE(
      chords_model_pointer->data(top_left_index_1, Qt::EditRole).toString(),
      "");
  QCOMPARE(
      chords_model_pointer->data(bottom_right_index_2, Qt::EditRole).toString(),
      "");
  song_editor.undo();

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
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column);

  QTest::newRow("chord then note")
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(1, 0, instrument_column)
      << song_editor.get_note_index(1, 0, interval_column);
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
  song_editor.trigger_delete();
  clear_selection();

  QCOMPARE(
      chords_model_pointer->data(top_left_index_1, Qt::EditRole).toString(),
      "");
  QCOMPARE(
      chords_model_pointer->data(bottom_right_index_3, Qt::EditRole).toString(),
      "");
  song_editor.undo();

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
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(1, 0, instrument_column)
      << song_editor.get_note_index(1, 0, interval_column);

  QTest::newRow("chord notes chord")
      << song_editor.get_chord_index(0, instrument_column)
      << song_editor.get_chord_index(0, interval_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column);
}

void Tester::test_paste_cell_template() {
  QFETCH(const QModelIndex, old_index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QModelIndex, new_index);
  QFETCH(const QVariant, new_value);

  selector_pointer->select(new_index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(old_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(old_index, Qt::EditRole), new_value);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(old_index, Qt::EditRole), old_value);
}

void Tester::test_paste_cell_template_data() {
  QTest::addColumn<QModelIndex>("old_index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QModelIndex>("new_index");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("chord interval")
      << song_editor.get_chord_index(0, interval_column)
      << QVariant::fromValue(Interval())
      << song_editor.get_chord_index(1, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("chord beats") << song_editor.get_chord_index(0, beats_column)
                               << QVariant::fromValue(Rational())
                               << song_editor.get_chord_index(1, beats_column)
                               << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord tempo ratio")
      << song_editor.get_chord_index(0, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << song_editor.get_chord_index(1, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord volume ratio")
      << song_editor.get_chord_index(0, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << song_editor.get_chord_index(1, volume_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("chord words")
      << song_editor.get_chord_index(0, words_column) << QVariant("")
      << song_editor.get_chord_index(1, words_column) << QVariant("hello");

  QTest::newRow("chord instrument")
      << song_editor.get_chord_index(0, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << song_editor.get_chord_index(1, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));

  QTest::newRow("note interval")
      << song_editor.get_note_index(0, 0, interval_column)
      << QVariant::fromValue(Interval())
      << song_editor.get_note_index(0, 1, interval_column)
      << QVariant::fromValue(Interval(2, 2, 1));

  QTest::newRow("note beats") << song_editor.get_note_index(0, 0, beats_column)
                              << QVariant::fromValue(Rational())
                              << song_editor.get_note_index(0, 1, beats_column)
                              << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note tempo ratio")
      << song_editor.get_note_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational())
      << song_editor.get_note_index(0, 1, tempo_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note volume ratio")
      << song_editor.get_note_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational())
      << song_editor.get_note_index(0, 1, volume_ratio_column)
      << QVariant::fromValue(Rational(2, 2));

  QTest::newRow("note words")
      << song_editor.get_note_index(0, 0, words_column) << QVariant("")
      << song_editor.get_note_index(0, 1, words_column) << QVariant("hello");

  QTest::newRow("note instrument")
      << song_editor.get_note_index(0, 0, instrument_column)
      << QVariant::fromValue(QVariant::fromValue(get_instrument_pointer("")))
      << song_editor.get_note_index(0, 1, instrument_column)
      << QVariant::fromValue(
             QVariant::fromValue(get_instrument_pointer("Oboe")));
}

void Tester::test_cut_paste_cell_template() {
  QFETCH(const QModelIndex, cut_index);
  QFETCH(const QModelIndex, paste_index);

  const auto cut_data = chords_model_pointer->data(cut_index);
  const auto paste_data = chords_model_pointer->data(paste_index);

  selector_pointer->select(cut_index, QItemSelectionModel::Select);
  song_editor.trigger_cut();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(cut_index).toString(), "");

  selector_pointer->select(paste_index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), cut_data);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_data);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->data(cut_index, Qt::EditRole), cut_data);
}

void Tester::test_cut_paste_cell_template_data() {
  QTest::addColumn<QModelIndex>("cut_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("chord interval")
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_chord_index(0, interval_column);
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
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(1, 0, instrument_column)
      << song_editor.get_note_index(1, 0, interval_column)
      << song_editor.get_chord_index(2, instrument_column)
      << song_editor.get_chord_index(2, interval_column);

  QTest::newRow("chord then note")
      << song_editor.get_chord_index(0, instrument_column)
      << song_editor.get_chord_index(0, interval_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 0, interval_column)
      << song_editor.get_chord_index(1, instrument_column)
      << song_editor.get_chord_index(1, interval_column)
      << song_editor.get_note_index(1, 0, instrument_column)
      << song_editor.get_note_index(1, 0, interval_column);
}

void Tester::test_paste_truncate_template() {
  QFETCH(const QModelIndex, copy_top_index);
  QFETCH(const QModelIndex, copy_bottom_index);
  QFETCH(const QModelIndex, paste_index);

  auto copy_top_data = chords_model_pointer->data(copy_top_index, Qt::EditRole);
  auto paste_data = chords_model_pointer->data(paste_index, Qt::EditRole);

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

  QCOMPARE(chords_model_pointer->data(paste_index, Qt::EditRole), paste_data);
};

void Tester::test_paste_truncate_template_data() {
  QTest::addColumn<QModelIndex>("copy_top_index");
  QTest::addColumn<QModelIndex>("copy_bottom_index");
  QTest::addColumn<QModelIndex>("paste_index");

  QTest::newRow("first notes")
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, instrument_column)
      << song_editor.get_note_index(0, 1, instrument_column);
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
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 0, instrument_column)
      << song_editor.get_note_index(0, 1, instrument_column);
};

void Tester::test_insert_delete() const {
  selector_pointer->select(song_editor.get_chord_index(2),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(song_editor.get_chord_index(2)), 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(song_editor.get_chord_index(2)), 0);

  // test chord templating from previous chord
  selector_pointer->select(song_editor.get_chord_index(1),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_chord_index(1, beats_column), Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  song_editor.undo();

  // test note templating from previous note
  selector_pointer->select(song_editor.get_note_index(0, 1),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(0, 2, beats_column), Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(
      chords_model_pointer->data(
          song_editor.get_note_index(0, 2, volume_ratio_column), Qt::EditRole),
      QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(
      chords_model_pointer->data(
          song_editor.get_note_index(0, 2, tempo_ratio_column), Qt::EditRole),
      QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(0, 2, words_column), Qt::EditRole),
           "hello");
  song_editor.undo();

  // test note inheritance from chord
  selector_pointer->select(song_editor.get_chord_index(1),
                           QItemSelectionModel::Select);
  song_editor.trigger_insert_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(1, 0, beats_column), Qt::EditRole),
           QVariant::fromValue(Rational(2, 2)));
  QCOMPARE(chords_model_pointer->data(
               song_editor.get_note_index(1, 0, words_column), Qt::EditRole),
           "hello");
  song_editor.undo();
}

void Tester::test_insert_rows_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const int, old_row_count);

  auto parent_index = chords_model_pointer->parent(index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_insert_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_insert_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("old_row_count");

  QTest::newRow("insert chord after") << song_editor.get_chord_index(0) << 3;

  QTest::newRow("insert note after") << song_editor.get_note_index(0, 0) << 2;
}

void Tester::test_delete_rows_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const int, old_row_count);

  auto parent_index = chords_model_pointer->parent(index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_delete();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count - 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), old_row_count);
}

void Tester::test_delete_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("old_row_count");

  QTest::newRow("delete chord") << song_editor.get_chord_index(0) << 3;

  QTest::newRow("delete note") << song_editor.get_note_index(0, 0) << 2;
}

void Tester::test_paste_rows_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const int, parent_row_count);

  auto parent_index = chords_model_pointer->parent(index);

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count + 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(parent_index), parent_row_count);
}

void Tester::test_paste_rows_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<int>("parent_row_count");

  QTest::newRow("paste chord after") << song_editor.get_chord_index(0) << 3;
  QTest::newRow("paste note after") << song_editor.get_note_index(0, 0) << 2;
}

void Tester::test_bad_paste_template() {
  QFETCH(const QString, copied);
  QFETCH(const QString, mime_type);
  QFETCH(const QModelIndex, index);
  QFETCH(const QString, error_message);

  close_message_later(error_message);
  copy_text(copied.toStdString(), mime_type);

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

void Tester::test_paste_rows() {
  // copy chord
  selector_pointer->select(song_editor.get_chord_index(0),
                           QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  // can't paste chord as a note
  close_message_later("Cannot paste chords into destination needing notes");

  selector_pointer->select(song_editor.get_note_index(0, 0),
                           QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();

  // copy note
  selector_pointer->select(song_editor.get_note_index(0, 0),
                           QItemSelectionModel::Select);
  song_editor.trigger_copy();
  clear_selection();

  // paste note into
  selector_pointer->select(song_editor.get_chord_index(2),
                           QItemSelectionModel::Select);
  song_editor.trigger_paste_into();
  clear_selection();

  QCOMPARE(chords_model_pointer->rowCount(song_editor.get_chord_index(2)), 1);
  song_editor.undo();
  QCOMPARE(chords_model_pointer->rowCount(song_editor.get_chord_index(2)), 0);

  // can't paste note as chord
  close_message_later("Cannot paste notes into destination needing chords");

  selector_pointer->select(song_editor.get_chord_index(0),
                           QItemSelectionModel::Select);
  song_editor.trigger_paste_cells_or_after();
  clear_selection();
}

void Tester::test_play() {
  // Test volume errors
  QVERIFY(chords_model_pointer->setData(
      song_editor.get_note_index(0, 0, volume_ratio_column),
      QVariant::fromValue(Rational(10)), Qt::EditRole));

  close_message_later(
      "Volume exceeds 100% for chord 1, note 1. Playing with 100% volume.");

  selector_pointer->select(song_editor.get_note_index(0, 0),
                           QItemSelectionModel::Select);
  song_editor.trigger_play();
  clear_selection();

  QThread::msleep(WAIT_TIME);
  song_editor.trigger_stop_playing();
  song_editor.undo();

  // Test midi overload
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

void Tester::test_expand_collapse_template() const {
  QFETCH(const QModelIndex, index);
  selector_pointer->select(index, QItemSelectionModel::Select);
  song_editor.trigger_collapse();
  song_editor.trigger_expand();
  Q_ASSERT(chords_view_pointer->isExpanded(index));
  song_editor.trigger_collapse();
  Q_ASSERT(!chords_view_pointer->isExpanded(index));
};

void Tester::test_expand_collapse_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::newRow("first chord") << song_editor.get_chord_index(0);
};

void Tester::test_io() {

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