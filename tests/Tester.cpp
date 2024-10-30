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
#include <QTemporaryFile>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

#include "justly/ChordColumn.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "justly/UnpitchedNoteColumn.hpp"
#include "justly/justly.hpp"

static const auto NUMBER_OF_CHORD_COLUMNS = 10;
static const auto NUMBER_OF_PITCHED_NOTE_COLUMNS = 5;
static const auto NUMBER_OF_UNPITCHED_NOTE_COLUMNS = 5;

static const auto BIG_VELOCITY = 126;

static const auto STARTING_KEY_1 = 401.0;
static const auto STARTING_KEY_2 = 402.0;

static const auto STARTING_TEMPO_1 = 150.0;
static const auto STARTING_TEMPO_2 = 100.0;

static const auto STARTING_VELOCITY_1 = 70;
static const auto STARTING_VELOCITY_2 = 80;

static const auto WAIT_TIME = 500;

static const auto OVERLOAD_NUMBER = 17;

static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;

static const auto A_MINUS_FREQUENCY = 216.8458;
static const auto A_PLUS_FREQUENCY = 223.2;
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

static const auto CHORDS_CELLS_MIME = "application/prs.chords_cells+json";
static const auto PITCHED_NOTES_CELLS_MIME = "application/prs.notes_cells+json";
static const auto UNPITCHED_NOTES_CELLS_MIME =
    "application/prs.percussions_cells+json";

static const auto *const SONG_TEXT = R""""({
    "chords": [
        {},
        {
            "beats": {
                "numerator": 3
            },
            "instrument": "Marimba",
            "interval": {
                "numerator": 3
            },
            "percussion_instrument": "Tambourine",
            "percussion_set": "Standard",
            "pitched_notes": [
                {
                    "instrument": "Marimba"
                },
                {
                    "beats": {
                        "numerator": 3
                    },
                    "instrument": "12-String Guitar",
                    "interval": {
                        "numerator": 3
                    },
                    "velocity_ratio": {
                        "numerator": 3
                    },
                    "words": "1"
                },
                {
                    "beats": {
                        "denominator": 2
                    },
                    "instrument": "Marimba",
                    "interval": {
                        "denominator": 2
                    },
                    "velocity_ratio": {
                        "denominator": 2
                    }
                },
                {
                    "beats": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "instrument": "Marimba",
                    "interval": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "velocity_ratio": {
                        "denominator": 2,
                        "numerator": 3
                    }
                },
                {
                    "instrument": "Marimba",
                    "interval": {
                        "octave": 1
                    }
                },
                {
                    "instrument": "Marimba",
                    "interval": {
                        "numerator": 3,
                        "octave": 1
                    }
                },
                {
                    "instrument": "Marimba",
                    "interval": {
                        "numerator": 2,
                        "octave": 1
                    }
                },
                {
                    "instrument": "Marimba",
                    "interval": {
                        "denominator": 2,
                        "numerator": 3,
                        "octave": 1
                    }
                }
            ],
            "tempo_ratio": {
                "numerator": 3
            },
            "unpitched_notes": [
                {
                    "percussion_instrument": "Tambourine",
                    "percussion_set": "Standard"
                },
                {
                    "beats": {
                        "numerator": 3
                    },
                    "percussion_instrument": "Acoustic or Low Bass Drum",
                    "percussion_set": "Brush",
                    "velocity_ratio": {
                        "numerator": 3
                    },
                    "words": "1"
                },
                {
                    "beats": {
                        "denominator": 2
                    },
                    "percussion_instrument": "Tambourine",
                    "percussion_set": "Standard",
                    "velocity_ratio": {
                        "denominator": 2
                    }
                },
                {
                    "beats": {
                        "denominator": 2,
                        "numerator": 3
                    },
                    "percussion_instrument": "Tambourine",
                    "percussion_set": "Standard",
                    "velocity_ratio": {
                        "denominator": 2,
                        "numerator": 3
                    }
                }
            ],
            "velocity_ratio": {
                "numerator": 3
            },
            "words": "1"
        },
        {
            "beats": {
                "denominator": 2
            },
            "interval": {
                "denominator": 2
            },
            "tempo_ratio": {
                "denominator": 2
            },
            "velocity_ratio": {
                "denominator": 2
            }
        },
        {
            "beats": {
                "denominator": 2,
                "numerator": 3
            },
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
            }
        },
        {
            "interval": {
                "octave": 1
            }
        },
        {
            "interval": {
                "numerator": 3,
                "octave": 1
            }
        },
        {
            "interval": {
                "denominator": 2,
                "octave": 1
            }
        },
        {
            "interval": {
                "denominator": 2,
                "numerator": 3,
                "octave": 1
            }
        }
    ],
    "gain": 1.0,
    "starting_key": 220.0,
    "starting_tempo": 100.0,
    "starting_velocity": 10.0
})"""";

struct TwoStringsRow {
  const QString first_string;
  const QString second_string;
};

struct ToStringRow {
  const QModelIndex index;
  const QString text;
};

struct CountRow {
  const int chord_number;
  const int number;
};

struct HeaderRow {
  int column_number;
  const QString column_name;
};

struct FlagRow {
  int column_number;
  bool is_editable;
};

struct FrequencyRow {
  double frequency;
  QString text;
};

struct TwoIndicesRow {
  QModelIndex first_index;
  QModelIndex second_index;
};

[[nodiscard]] static auto
get_selector_pointer(const QAbstractItemView *table_view_pointer) {
  auto *selector_pointer = table_view_pointer->selectionModel();
  Q_ASSERT(selector_pointer != nullptr);
  return selector_pointer;
}

static void clear_selection(QItemSelectionModel *selector_pointer) {
  Q_ASSERT(selector_pointer != nullptr);
  selector_pointer->select(QModelIndex(), QItemSelectionModel::Clear);
}

[[nodiscard]] static auto get_indices(QAbstractItemModel *model_pointer,
                                      int item_number, int number_of_columns) {
  Q_ASSERT(model_pointer != nullptr);

  std::vector<QModelIndex> indices;
  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    indices.push_back(model_pointer->index(item_number, column_number));
  }
  return indices;
}

[[nodiscard]] static auto get_index_pairs(QAbstractItemModel *model_pointer,
                                          int first_row_number,
                                          int second_row_number,
                                          int number_of_columns) {
  Q_ASSERT(model_pointer != nullptr);

  std::vector<TwoIndicesRow> rows;
  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    rows.push_back({model_pointer->index(first_row_number, column_number),
                    model_pointer->index(second_row_number, column_number)});
  }
  return rows;
}

static void delete_cell(SongEditor *song_editor_pointer,
                        const QModelIndex &index) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));
  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_delete(song_editor_pointer);
  clear_selection(selector_pointer);
}

static void play_cell(SongEditor *song_editor_pointer,
                      const QModelIndex &index) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));
  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_play(song_editor_pointer);
  clear_selection(selector_pointer);
}

static void open_text(SongEditor *song_editor_pointer,
                      const QString &json_song) {
  QTemporaryFile json_file;
  QVERIFY(json_file.open());
  json_file.write(json_song.toStdString().c_str());
  json_file.close();
  open_file(song_editor_pointer, json_file.fileName());
}

static void test_column_flags_editable(const QAbstractItemModel *model_pointer,
                                       int column, bool is_editable) {
  Q_ASSERT(model_pointer != nullptr);

  auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  QCOMPARE(model_pointer->index(0, column).flags(),
           is_editable ? (uneditable_flags | Qt::ItemIsEditable)
                       : uneditable_flags);
}

static void test_number_of_columns(const QAbstractItemModel *model_pointer,
                                   int number_of_columns) {
  QCOMPARE(model_pointer->columnCount(), number_of_columns);
}

static void test_set_values(const SongEditor *song_editor_pointer,
                            const std::vector<TwoIndicesRow> &rows) {
  const auto *table_view_pointer = get_table_view_pointer(song_editor_pointer);
  for (const auto &row : rows) {

    const auto &copy_index = row.first_index;
    const auto &paste_index = row.second_index;

    auto copy_value = copy_index.data();
    auto paste_value = paste_index.data();
    QCOMPARE_NE(copy_value, paste_value);

    auto *cell_editor_pointer = create_editor(table_view_pointer, paste_index);
    QCOMPARE(cell_editor_pointer->property(
                 cell_editor_pointer->metaObject()->userProperty().name()),
             paste_value);

    set_editor(table_view_pointer, cell_editor_pointer, paste_index,
               copy_value);

    QCOMPARE(paste_index.data(), copy_value);
    undo(song_editor_pointer);
    QCOMPARE(paste_index.data(), paste_value);
  }
}

static void test_get_unsupported_role(const QAbstractItemModel *model_pointer) {
  Q_ASSERT(model_pointer != nullptr);
  QCOMPARE(model_pointer->index(0, 0).data(Qt::DecorationRole), QVariant());
}

static void test_set_unsupported_role(QAbstractItemModel *model_pointer) {
  Q_ASSERT(model_pointer != nullptr);
  QVERIFY(!(model_pointer->setData(model_pointer->index(0, 1), QVariant(),
                                   Qt::DecorationRole)));
};

static void test_column_headers(const QAbstractItemModel *model_pointer,
                                const std::vector<HeaderRow> &rows) {
  Q_ASSERT(model_pointer != nullptr);
  for (const auto &row : rows) {
    QCOMPARE(model_pointer->headerData(row.column_number, Qt::Horizontal),
             row.column_name);
  }
}

static void test_delete_cells(SongEditor *song_editor_pointer,
                              const std::vector<QModelIndex> &indices) {
  for (const auto &index : indices) {
    const auto &old_value = index.data();

    delete_cell(song_editor_pointer, index);

    QCOMPARE_NE(index.data(), old_value);
    undo(song_editor_pointer);
    QCOMPARE(index.data(), old_value);
  }
}

static void test_copy_paste_cells(SongEditor *song_editor_pointer,
                                  const std::vector<TwoIndicesRow> &rows) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  for (const auto &row : rows) {
    const auto &copy_index = row.first_index;
    const auto &paste_index = row.second_index;

    auto copy_value = copy_index.data();
    auto paste_value = paste_index.data();

    QCOMPARE_NE(copy_value, paste_value);

    selector_pointer->select(copy_index, QItemSelectionModel::Select);
    trigger_copy(song_editor_pointer);
    clear_selection(selector_pointer);

    selector_pointer->select(paste_index, QItemSelectionModel::Select);
    trigger_paste_over(song_editor_pointer);
    clear_selection(selector_pointer);

    QCOMPARE(paste_index.data(), copy_value);
    undo(song_editor_pointer);
    QCOMPARE(paste_index.data(), paste_value);
  }
}

static void test_cut_paste_cells(SongEditor *song_editor_pointer,
                                 const std::vector<TwoIndicesRow> &rows) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  for (const auto &row : rows) {
    const auto &cut_index = row.first_index;
    const auto &paste_index = row.second_index;

    const auto cut_value = cut_index.data();
    const auto paste_value = paste_index.data();
    QCOMPARE_NE(cut_value, paste_value);

    selector_pointer->select(cut_index, QItemSelectionModel::Select);
    trigger_cut(song_editor_pointer);
    clear_selection(selector_pointer);

    QCOMPARE_NE(cut_index.data(), cut_value);

    selector_pointer->select(paste_index, QItemSelectionModel::Select);
    trigger_paste_over(song_editor_pointer);
    clear_selection(selector_pointer);

    QCOMPARE(paste_index.data(), cut_value);
    undo(song_editor_pointer);
    QCOMPARE(paste_index.data(), paste_value);
    undo(song_editor_pointer);
    QCOMPARE(cut_index.data(), cut_value);
  }
}

static void test_copy_paste_insert_rows(SongEditor *song_editor_pointer,
                                        QAbstractItemModel *model_pointer) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));
  selector_pointer->select(model_pointer->index(0, 0),
                           QItemSelectionModel::Select);
  trigger_copy(song_editor_pointer);
  clear_selection(selector_pointer);

  auto number_of_rows = model_pointer->rowCount();
  trigger_paste_into(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), number_of_rows + 1);
  undo(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), number_of_rows);

  selector_pointer->select(model_pointer->index(0, 0),
                           QItemSelectionModel::Select);
  trigger_paste_after(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), number_of_rows + 1);
  undo(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), number_of_rows);
  clear_selection(selector_pointer);
}

static void test_insert_into(SongEditor *song_editor_pointer,
                             QAbstractItemModel *model_pointer) {
  Q_ASSERT(model_pointer != nullptr);

  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  auto chord_index = model_pointer->index(0, chord_interval_column);
  auto old_row_count = model_pointer->rowCount(chord_index);
  selector_pointer->select(chord_index, QItemSelectionModel::Select);
  trigger_insert_into(song_editor_pointer);
  clear_selection(selector_pointer);

  QCOMPARE(model_pointer->rowCount(chord_index), old_row_count + 1);
  undo(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(chord_index), old_row_count);
}

static void test_insert_after(SongEditor *song_editor_pointer,
                              QAbstractItemModel *model_pointer) {
  Q_ASSERT(model_pointer != nullptr);

  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  auto index = model_pointer->index(0, 0);
  auto old_row_count = model_pointer->rowCount();

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_insert_after(song_editor_pointer);
  clear_selection(selector_pointer);

  QCOMPARE(model_pointer->rowCount(), old_row_count + 1);
  undo(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), old_row_count);
}

static void test_remove_rows(SongEditor *song_editor_pointer,
                             QAbstractItemModel *model_pointer) {
  Q_ASSERT(model_pointer != nullptr);

  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  auto index = model_pointer->index(0, 0);
  auto old_row_count = model_pointer->rowCount();

  selector_pointer->select(index, QItemSelectionModel::Select);
  trigger_remove_rows(song_editor_pointer);
  clear_selection(selector_pointer);

  QCOMPARE(model_pointer->rowCount(), old_row_count - 1);
  undo(song_editor_pointer);
  QCOMPARE(model_pointer->rowCount(), old_row_count);
}

static void test_plays(SongEditor *song_editor_pointer,
                       const std::vector<TwoIndicesRow> &rows) {
  for (const auto &row : rows) {
    auto *selector_pointer =
        get_selector_pointer(get_table_view_pointer(song_editor_pointer));

    selector_pointer->select(QItemSelection(row.first_index, row.second_index),
                             QItemSelectionModel::Select);
    trigger_play(song_editor_pointer);
    // first cut off early
    trigger_play(song_editor_pointer);
    // now play for a while
    QThread::msleep(WAIT_TIME);
    trigger_stop_playing(song_editor_pointer);

    clear_selection(selector_pointer);
  }
}

void Tester::close_message_later(const QString &expected_text) {
  auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto *timer_pointer = // NOLINT(cppcoreguidelines-owning-memory)
      new QTimer(this);
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

Tester::Tester() : song_editor_pointer(make_song_editor()) {}

Tester::~Tester() { delete_song_editor(song_editor_pointer); }

void Tester::test_bad_pastes(const QModelIndex &index,
                             const std::vector<BadPasteRow> &rows) {
  auto *selector_pointer =
      get_selector_pointer(get_table_view_pointer(song_editor_pointer));

  for (const auto &row : rows) {
    auto *new_data_pointer = // NOLINT(cppcoreguidelines-owning-memory)
        new QMimeData;

    Q_ASSERT(new_data_pointer != nullptr);
    new_data_pointer->setData(row.mime_type, row.copied.toStdString().c_str());

    auto *clipboard_pointer = QGuiApplication::clipboard();
    Q_ASSERT(clipboard_pointer != nullptr);
    clipboard_pointer->setMimeData(new_data_pointer);

    selector_pointer->select(index, QItemSelectionModel::Select);
    close_message_later(row.error_message);
    trigger_paste_over(song_editor_pointer);
    clear_selection(selector_pointer);
  }
}

void Tester::initTestCase() const {
  register_converters();
  open_text(song_editor_pointer, SONG_TEXT);
}

void Tester::test_to_strings() const {
  const auto *chords_model_pointer =
      get_chords_model_pointer(song_editor_pointer);
  for (const auto &row : std::vector({
           ToStringRow(
               {chords_model_pointer->index(0, chord_instrument_column), ""}),
           ToStringRow(
               {chords_model_pointer->index(0, chord_percussion_set_column),
                ""}),
           ToStringRow({chords_model_pointer->index(
                            0, chord_percussion_instrument_column),
                        ""}),
           ToStringRow(
               {chords_model_pointer->index(0, chord_interval_column), ""}),
           ToStringRow(
               {chords_model_pointer->index(1, chord_interval_column), "3"}),
           ToStringRow(
               {chords_model_pointer->index(2, chord_interval_column), "/2"}),
           ToStringRow(
               {chords_model_pointer->index(3, chord_interval_column), "3/2"}),
           ToStringRow(
               {chords_model_pointer->index(4, chord_interval_column), "o1"}),
           ToStringRow(
               {chords_model_pointer->index(5, chord_interval_column), "3o1"}),
           ToStringRow(
               {chords_model_pointer->index(6, chord_interval_column), "/2o1"}),
           ToStringRow({chords_model_pointer->index(7, chord_interval_column),
                        "3/2o1"}),
           ToStringRow(
               {chords_model_pointer->index(0, chord_beats_column), ""}),
           ToStringRow(
               {chords_model_pointer->index(1, chord_beats_column), "3"}),
           ToStringRow(
               {chords_model_pointer->index(2, chord_beats_column), "/2"}),
           ToStringRow(
               {chords_model_pointer->index(3, chord_beats_column), "3/2"}),
       })) {
    QCOMPARE(row.index.data().toString(), row.text);
  }
}

void Tester::test_chords_count() const {
  QCOMPARE(get_chords_model_pointer(song_editor_pointer)->rowCount(), 8);
}

void Tester::test_pitched_notes_count() const {
  const auto *pitched_notes_model_pointer =
      get_pitched_notes_model_pointer(song_editor_pointer);
  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 8}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    trigger_edit_pitched_notes(song_editor_pointer, row.chord_number);
    QCOMPARE(pitched_notes_model_pointer->rowCount(), row.number);
    undo(song_editor_pointer);
  }
}

void Tester::test_unpitched_notes_count() const {
  const auto *unpitched_notes_model_pointer =
      get_unpitched_notes_model_pointer(song_editor_pointer);
  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 4}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    trigger_edit_unpitched_notes(song_editor_pointer, row.chord_number);
    QCOMPARE(unpitched_notes_model_pointer->rowCount(), row.number);
    undo(song_editor_pointer);
  }
}

void Tester::test_back_to_chords() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 0);
  trigger_back_to_chords(song_editor_pointer);
  undo(song_editor_pointer);
  undo(song_editor_pointer);
}

void Tester::test_number_of_chord_columns() const {
  test_number_of_columns(get_chords_model_pointer(song_editor_pointer),
                         NUMBER_OF_CHORD_COLUMNS);
}

void Tester::test_number_of_pitched_note_columns() const {
  test_number_of_columns(get_pitched_notes_model_pointer(song_editor_pointer),
                         NUMBER_OF_PITCHED_NOTE_COLUMNS);
}

void Tester::test_number_of_unpitched_note_columns() const {
  test_number_of_columns(get_unpitched_notes_model_pointer(song_editor_pointer),
                         NUMBER_OF_UNPITCHED_NOTE_COLUMNS);
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

void Tester::test_row_headers() const {
  const auto *chords_model_pointer =
      get_chords_model_pointer(song_editor_pointer);
  QCOMPARE(chords_model_pointer->headerData(0, Qt::Vertical), QVariant(1));
  QCOMPARE(
      chords_model_pointer->headerData(0, Qt::Vertical, Qt::DecorationRole),
      QVariant());
}

void Tester::test_chord_column_headers() const {
  test_column_headers(
      get_chords_model_pointer(song_editor_pointer),
      std::vector(
          {HeaderRow({chord_instrument_column, "Instrument"}),
           HeaderRow({chord_percussion_set_column, "Percussion set"}),
           HeaderRow(
               {chord_percussion_instrument_column, "Percussion instrument"}),
           HeaderRow({chord_interval_column, "Interval"}),
           HeaderRow({chord_beats_column, "Beats"}),
           HeaderRow({chord_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({chord_tempo_ratio_column, "Tempo ratio"}),
           HeaderRow({chord_words_column, "Words"}),
           HeaderRow({chord_pitched_notes_column, "Pitched notes"}),
           HeaderRow({chord_unpitched_notes_column, "Unpitched notes"})}));
}

void Tester::test_pitched_note_column_headers() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_column_headers(
      get_pitched_notes_model_pointer(song_editor_pointer),
      std::vector(
          {HeaderRow({pitched_note_instrument_column, "Instrument"}),
           HeaderRow({pitched_note_interval_column, "Interval"}),
           HeaderRow({pitched_note_beats_column, "Beats"}),
           HeaderRow({pitched_note_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({pitched_note_words_column, "Words"})}));
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_column_headers() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_column_headers(
      get_unpitched_notes_model_pointer(song_editor_pointer),
      std::vector(
          {HeaderRow({unpitched_note_percussion_set_column, "Percussion set"}),
           HeaderRow({unpitched_note_percussion_instrument_column,
                      "Percussion instrument"}),
           HeaderRow({unpitched_note_beats_column, "Beats"}),
           HeaderRow({unpitched_note_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({unpitched_note_words_column, "Words"})}));
  undo(song_editor_pointer);
}

void Tester::test_chord_flags() const {
  const auto *chords_model_pointer =
      get_chords_model_pointer(song_editor_pointer);
  for (const auto &row :
       std::vector({FlagRow({chord_interval_column, true}),
                    FlagRow({chord_pitched_notes_column, false}),
                    FlagRow({chord_unpitched_notes_column, false})})) {
    test_column_flags_editable(chords_model_pointer, row.column_number,
                               row.is_editable);
  }
}

void Tester::test_pitched_note_flags() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_column_flags_editable(
      get_pitched_notes_model_pointer(song_editor_pointer),
      pitched_note_interval_column, true);
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_flags() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_column_flags_editable(
      get_unpitched_notes_model_pointer(song_editor_pointer),
      unpitched_note_percussion_set_column, true);
  undo(song_editor_pointer);
}

void Tester::test_chord_frequencies() const {
  const auto *chords_model_pointer =
      get_chords_model_pointer(song_editor_pointer);
  for (const auto &row : std::vector({
           FrequencyRow({A_MINUS_FREQUENCY, "216.8 Hz; A3 − 25 cents"}),
           FrequencyRow({A_PLUS_FREQUENCY, "223.2 Hz; A3 + 25 cents"}),
           FrequencyRow({A_FREQUENCY, "220 Hz; A3"}),
           FrequencyRow({B_FLAT_FREQUENCY, "233.1 Hz; B♭3"}),
           FrequencyRow({B_FREQUENCY, "246.9 Hz; B3"}),
           FrequencyRow({C_FREQUENCY, "261.6 Hz; C4"}),
           FrequencyRow({C_SHARP_FREQUENCY, "277.2 Hz; C♯4"}),
           FrequencyRow({D_FREQUENCY, "293.7 Hz; D4"}),
           FrequencyRow({E_FLAT_FREQUENCY, "311.1 Hz; E♭4"}),
           FrequencyRow({E_FREQUENCY, "329.6 Hz; E4"}),
           FrequencyRow({F_FREQUENCY, "349.2 Hz; F4"}),
           FrequencyRow({F_SHARP_FREQUENCY, "370 Hz; F♯4"}),
           FrequencyRow({G_FREQUENCY, "392 Hz; G4"}),
           FrequencyRow({A_FLAT_FREQUENCY, "415.3 Hz; A♭4"}),
       })) {
    set_starting_key(song_editor_pointer, row.frequency);
    QCOMPARE(chords_model_pointer->index(0, chord_interval_column)
                 .data(Qt::StatusTipRole),
             row.text);
    undo(song_editor_pointer);
  }
}

void Tester::test_pitched_note_frequencies() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  set_starting_key(song_editor_pointer, A_FREQUENCY);
  QCOMPARE(get_pitched_notes_model_pointer(song_editor_pointer)
               ->index(0, chord_interval_column)
               .data(Qt::StatusTipRole),
           "660 Hz; E5 + 2 cents");
  undo(song_editor_pointer);
  undo(song_editor_pointer);
}

void Tester::test_unpitched_status() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  QCOMPARE(get_unpitched_notes_model_pointer(song_editor_pointer)
               ->index(0, chord_interval_column)
               .data(Qt::StatusTipRole),
           "");
  undo(song_editor_pointer);
}

void Tester::test_get_unsupported_chord_role() const {
  test_get_unsupported_role(get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_get_unsupported_pitched_note_role() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_get_unsupported_role(
      get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_get_unsupported_unpitched_note_role() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_get_unsupported_role(
      get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_set_chord_values() const {
  test_set_values(song_editor_pointer,
                  get_index_pairs(get_chords_model_pointer(song_editor_pointer),
                                  0, 1, NUMBER_OF_CHORD_COLUMNS - 2));
}

void Tester::test_set_pitched_note_values() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_set_values(
      song_editor_pointer,
      get_index_pairs(get_pitched_notes_model_pointer(song_editor_pointer), 0,
                      1, NUMBER_OF_PITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_set_unpitched_note_values() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_set_values(
      song_editor_pointer,
      get_index_pairs(get_unpitched_notes_model_pointer(song_editor_pointer), 0,
                      1, NUMBER_OF_UNPITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_set_unsupported_chord_role() const {
  test_set_unsupported_role(get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_set_unsupported_pitched_note_role() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_set_unsupported_role(
      get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_set_unsupported_unpitched_note_role() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_set_unsupported_role(
      get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_delete_chord_cells() const {
  test_delete_cells(song_editor_pointer,
                    get_indices(get_chords_model_pointer(song_editor_pointer),
                                1, NUMBER_OF_CHORD_COLUMNS));
}

void Tester::test_delete_pitched_note_cells() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_delete_cells(
      song_editor_pointer,
      get_indices(get_pitched_notes_model_pointer(song_editor_pointer), 1,
                  NUMBER_OF_PITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_delete_unpitched_note_cells() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_delete_cells(
      song_editor_pointer,
      get_indices(get_unpitched_notes_model_pointer(song_editor_pointer), 1,
                  NUMBER_OF_UNPITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_copy_paste_chord_cells() const {
  test_copy_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_chords_model_pointer(song_editor_pointer), 0, 1,
                      NUMBER_OF_CHORD_COLUMNS));
}

void Tester::test_copy_paste_pitched_note_cells() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_copy_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_pitched_notes_model_pointer(song_editor_pointer), 0,
                      1, NUMBER_OF_PITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_copy_paste_unpitched_note_cells() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_copy_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_unpitched_notes_model_pointer(song_editor_pointer), 0,
                      1, NUMBER_OF_UNPITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_cut_paste_chord_cells() const {
  test_cut_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_chords_model_pointer(song_editor_pointer), 1, 0,
                      NUMBER_OF_CHORD_COLUMNS));
}

void Tester::test_cut_paste_pitched_note_cells() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_cut_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_pitched_notes_model_pointer(song_editor_pointer), 1,
                      0, NUMBER_OF_PITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_cut_paste_unpitched_note_cells() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_cut_paste_cells(
      song_editor_pointer,
      get_index_pairs(get_unpitched_notes_model_pointer(song_editor_pointer), 1,
                      0, NUMBER_OF_UNPITCHED_NOTE_COLUMNS));
  undo(song_editor_pointer);
}

void Tester::test_copy_paste_insert_chord() const {
  test_copy_paste_insert_rows(song_editor_pointer,
                              get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_copy_paste_insert_pitched_note() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_copy_paste_insert_rows(
      song_editor_pointer,
      get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_copy_paste_insert_unpitched_note() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_copy_paste_insert_rows(
      song_editor_pointer,
      get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_chord_insert_into() const {
  test_insert_into(song_editor_pointer,
                   get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_pitched_note_insert_into() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_insert_into(song_editor_pointer,
                   get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_insert_into() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_insert_into(song_editor_pointer,
                   get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_chord_insert_after() const {
  test_insert_after(song_editor_pointer,
                    get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_pitched_note_insert_after() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_insert_after(song_editor_pointer,
                    get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_insert_after() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_insert_after(song_editor_pointer,
                    get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_chord_remove_rows() const {
  test_remove_rows(song_editor_pointer,
                   get_chords_model_pointer(song_editor_pointer));
}

void Tester::test_pitched_note_remove_rows() const {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_remove_rows(song_editor_pointer,
                   get_pitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_remove_rows() const {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_remove_rows(song_editor_pointer,
                   get_unpitched_notes_model_pointer(song_editor_pointer));
  undo(song_editor_pointer);
}

void Tester::test_bad_chord_pastes() {
  test_bad_pastes(
      get_chords_model_pointer(song_editor_pointer)->index(0, 0),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime into destination needing "
                        "chords cells"}),
           BadPasteRow(
               {"", PITCHED_NOTES_CELLS_MIME,
                "Cannot paste pitched notes cells into destination needing "
                "chords cells"}),
           BadPasteRow(
               {"[", CHORDS_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", CHORDS_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", CHORDS_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}));
}

void Tester::test_bad_pitched_note_pastes() {
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_bad_pastes(
      get_pitched_notes_model_pointer(song_editor_pointer)->index(0, 0),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime into destination needing "
                        "pitched notes cells"}),
           BadPasteRow({"", CHORDS_CELLS_MIME,
                        "Cannot paste chords cells into destination needing "
                        "pitched notes cells"}),
           BadPasteRow(
               {"[", PITCHED_NOTES_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", PITCHED_NOTES_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", PITCHED_NOTES_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}));
  undo(song_editor_pointer);
}

void Tester::test_bad_unpitched_note_pastes() {
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_bad_pastes(
      get_unpitched_notes_model_pointer(song_editor_pointer)->index(0, 0),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime into destination needing "
                        "unpitched notes cells"}),
           BadPasteRow({"", CHORDS_CELLS_MIME,
                        "Cannot paste chords cells into destination needing "
                        "unpitched notes cells"}),
           BadPasteRow(
               {"[", UNPITCHED_NOTES_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", UNPITCHED_NOTES_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", UNPITCHED_NOTES_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}));
  undo(song_editor_pointer);
}

void Tester::test_too_loud() {
  trigger_edit_pitched_notes(song_editor_pointer, 1);

  set_starting_velocity(song_editor_pointer, BIG_VELOCITY);

  close_message_later(
      "Velocity 378 exceeds 127 for chord 2, pitched note 1. Playing "
      "with velocity 127.");

  play_cell(song_editor_pointer,
            get_pitched_notes_model_pointer(song_editor_pointer)
                ->index(0, pitched_note_interval_column));

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor_pointer);
  undo(song_editor_pointer);

  undo(song_editor_pointer);
}

void Tester::test_too_many_channels() {
  trigger_edit_pitched_notes(song_editor_pointer, 2);

  for (auto number = 0; number < OVERLOAD_NUMBER; number = number + 1) {
    trigger_insert_into(song_editor_pointer);
  }

  trigger_back_to_chords(song_editor_pointer);

  close_message_later("Out of MIDI channels for chord 3, pitched note 17. Not "
                      "playing pitched note.");

  play_cell(song_editor_pointer, get_chords_model_pointer(song_editor_pointer)
                                     ->index(2, chord_interval_column));

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor_pointer);

  // undo back to chords
  undo(song_editor_pointer);

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    undo(song_editor_pointer);
  }

  // undo edit pitched_notes
  undo(song_editor_pointer);
}

void Tester::test_missing_instruments() {
  auto *chords_model_pointer = get_chords_model_pointer(song_editor_pointer);
  auto *pitched_notes_model_pointer =
      get_pitched_notes_model_pointer(song_editor_pointer);
  auto *unpitched_notes_model_pointer =
      get_unpitched_notes_model_pointer(song_editor_pointer);

  delete_cell(song_editor_pointer,
              chords_model_pointer->index(1, chord_instrument_column));
  delete_cell(song_editor_pointer,
              chords_model_pointer->index(1, chord_percussion_set_column));
  delete_cell(song_editor_pointer, chords_model_pointer->index(
                                       1, chord_percussion_instrument_column));

  trigger_edit_pitched_notes(song_editor_pointer, 1);

  delete_cell(song_editor_pointer, pitched_notes_model_pointer->index(
                                       0, pitched_note_instrument_column));
  QCOMPARE(pitched_notes_model_pointer
               ->data(pitched_notes_model_pointer->index(
                   0, pitched_note_instrument_column))
               .toString(),
           "");
  close_message_later(
      "No instrument for chord 2, pitched note 1. Using Marimba.");

  play_cell(song_editor_pointer, pitched_notes_model_pointer->index(
                                     0, pitched_note_instrument_column));

  // undo delete pitched_note instrument
  undo(song_editor_pointer);
  // undo edit pitched_notes
  undo(song_editor_pointer);

  trigger_edit_unpitched_notes(song_editor_pointer, 1);

  delete_cell(song_editor_pointer,
              unpitched_notes_model_pointer->index(
                  0, unpitched_note_percussion_set_column));

  close_message_later(
      "No percussion set for chord 2, unpitched note 1. Using Standard.");

  play_cell(song_editor_pointer, unpitched_notes_model_pointer->index(
                                     0, unpitched_note_percussion_set_column));
  // undo edit delete unpitched_note set
  undo(song_editor_pointer);

  delete_cell(song_editor_pointer,
              unpitched_notes_model_pointer->index(
                  0, unpitched_note_percussion_instrument_column));

  close_message_later("No percussion instrument for chord 2, "
                      "unpitched note 1. Using Tambourine.");

  play_cell(song_editor_pointer, unpitched_notes_model_pointer->index(
                                     0, unpitched_note_percussion_set_column));
  // undo delete unpitched_note instrument
  undo(song_editor_pointer);
  // undo edit unpitched_notes
  undo(song_editor_pointer);
  // undo delete chord unpitched_note instrument
  undo(song_editor_pointer);
  // undo delete chord unpitched_note set
  undo(song_editor_pointer);
  // undo delete chord instrument
  undo(song_editor_pointer);
}

void Tester::test_chord_plays() const {
  const auto *chords_model_pointer =
      get_chords_model_pointer(song_editor_pointer);
  test_plays(song_editor_pointer,
             std::vector({
                 TwoIndicesRow(
                     {chords_model_pointer->index(0, chord_interval_column),
                      chords_model_pointer->index(1, chord_interval_column)}),
                 TwoIndicesRow(
                     {chords_model_pointer->index(1, chord_interval_column),
                      chords_model_pointer->index(1, chord_interval_column)}),
             }));
}

void Tester::test_pitched_note_plays() const {
  const auto *pitched_notes_model_pointer =
      get_pitched_notes_model_pointer(song_editor_pointer);
  trigger_edit_pitched_notes(song_editor_pointer, 1);
  test_plays(song_editor_pointer,
             std::vector({
                 TwoIndicesRow({pitched_notes_model_pointer->index(
                                    0, pitched_note_interval_column),
                                pitched_notes_model_pointer->index(
                                    1, pitched_note_interval_column)}),
                 TwoIndicesRow({pitched_notes_model_pointer->index(
                                    1, pitched_note_interval_column),
                                pitched_notes_model_pointer->index(
                                    1, pitched_note_interval_column)}),
             }));
  undo(song_editor_pointer);
}

void Tester::test_unpitched_note_plays() const {
  const auto *unpitched_notes_model_pointer =
      get_unpitched_notes_model_pointer(song_editor_pointer);
  trigger_edit_unpitched_notes(song_editor_pointer, 1);
  test_plays(song_editor_pointer,
             std::vector({
                 TwoIndicesRow({unpitched_notes_model_pointer->index(
                                    0, unpitched_note_percussion_set_column),
                                unpitched_notes_model_pointer->index(
                                    1, unpitched_note_percussion_set_column)}),
                 TwoIndicesRow({unpitched_notes_model_pointer->index(
                                    1, unpitched_note_percussion_set_column),
                                unpitched_notes_model_pointer->index(
                                    1, unpitched_note_percussion_set_column)}),
             }));
  undo(song_editor_pointer);
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

void Tester::test_broken_file() {
  for (const auto &row : std::vector({
           TwoStringsRow(
               {"{", "[json.exception.parse_error.101] parse error at line 1, "
                     "column 2: syntax error while parsing object key - "
                     "unexpected end of input; expected string literal"}),
           TwoStringsRow({"[]", "At  of [] - unexpected instance type\n"}),
           TwoStringsRow({"[1]", "At  of [1] - unexpected instance type\n"}),
       })) {
    close_message_later(row.second_string);
    open_text(song_editor_pointer, row.first_string);
  }
}

void Tester::test_open_empty() const {
  open_text(song_editor_pointer, R""""({
    "gain": 5.0,
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_velocity": 64
})"""");
}