#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QMetaObject>
#include <QtCore/QMimeData>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTemporaryFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtTest/QTest>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <string>
#include <vector>

// IWYU pragma: no_include <algorithm>
// IWYU pragma: no_include <utility>

#include "justly/justly.hpp"

struct SongEditor;

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

static const auto A_MINUS_FREQUENCY = 217;
static const auto A_PLUS_FREQUENCY = 223;
static const auto A_FREQUENCY = 220.0;
static const auto B_FLAT_FREQUENCY = 233;
static const auto B_FREQUENCY = 247;
static const auto C_FREQUENCY = 262;
static const auto C_SHARP_FREQUENCY = 277;
static const auto D_FREQUENCY = 294;
static const auto E_FLAT_FREQUENCY = 311;
static const auto E_FREQUENCY = 330;
static const auto F_FREQUENCY = 349;
static const auto F_SHARP_FREQUENCY = 370;
static const auto G_FREQUENCY = 392;
static const auto A_FLAT_FREQUENCY = 415;

static const auto CHORDS_CELLS_MIME = "application/prs.chords_cells+json";
static const auto PITCHED_NOTES_CELLS_MIME =
    "application/prs.pitched_notes_cells+json";
static const auto UNPITCHED_NOTES_CELLS_MIME =
    "application/prs.unpitched_notes_cells+json";

static const auto SONG_TEXT = R""""({
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

struct BadPasteRow {
  const QString copied;
  const QString mime_type;
  const QString error_message;
};

[[nodiscard]] static auto
get_selector(const QAbstractItemView &table_view) -> QItemSelectionModel & {
  const auto &selector = table_view.selectionModel();
  Q_ASSERT(selector != nullptr);
  return *selector;
}

static void clear_selection(QItemSelectionModel &selector) {
  selector.select(QModelIndex(), QItemSelectionModel::Clear);
}

[[nodiscard]] static auto get_index_pairs(QAbstractItemModel &model,
                                          int first_row_number,
                                          int second_row_number,
                                          int number_of_columns) {
  std::vector<TwoIndicesRow> rows;
  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    rows.push_back({model.index(first_row_number, column_number),
                    model.index(second_row_number, column_number)});
  }
  return rows;
}

static void delete_cell(SongEditor &song_editor, const QModelIndex &index) {
  auto &selector = get_selector(get_table_view(song_editor));
  selector.select(index, QItemSelectionModel::Select);
  trigger_delete_cells(song_editor);
  clear_selection(selector);
}

static void play_cell(SongEditor &song_editor, const QModelIndex &index) {
  auto &selector = get_selector(get_table_view(song_editor));
  selector.select(index, QItemSelectionModel::Select);
  trigger_play(song_editor);
  clear_selection(selector);
}

static void open_text(SongEditor &song_editor, const QString &json_song) {
  QTemporaryFile json_file;
  QVERIFY(json_file.open());
  json_file.write(json_song.toStdString().c_str());
  json_file.close();
  open_file(song_editor, json_file.fileName());
}

static void test_number_of_columns(const QAbstractItemModel &model,
                                   int number_of_columns) {
  QCOMPARE(model.columnCount(), number_of_columns);
}

struct Tester : public QObject {
  Q_OBJECT

public:
  bool waiting_for_message = false;

private slots:
  void run_tests();
};

static void close_message_later(Tester &tester, const QString &expected_text) {
  auto waiting_before = tester.waiting_for_message;
  tester.waiting_for_message = true;
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&tester));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &tester, [&tester, expected_text]() {
        for (auto *const widget_pointer : QApplication::topLevelWidgets()) {
          auto *box_pointer = dynamic_cast<QMessageBox *>(widget_pointer);
          if (box_pointer != nullptr) {
            auto actual_text = box_pointer->text();
            tester.waiting_for_message = false;
            QTest::keyEvent(QTest::Press, box_pointer, Qt::Key_Enter);
            QCOMPARE(actual_text, expected_text);
            break;
          }
        }
      });
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);
};

static void test_model(Tester &tester, SongEditor &song_editor,
                       QAbstractItemModel &model,
                       const std::vector<HeaderRow> &column_header_rows,
                       const std::vector<FlagRow> &flag_rows,
                       const std::vector<TwoIndicesRow> &play_rows,
                       const std::vector<BadPasteRow> &bad_paste_rows,
                       int empty_row_number, int full_row_number,
                       int number_of_columns, int max_set_column) {
  for (const auto &row : column_header_rows) {
    QCOMPARE(model.headerData(row.column_number, Qt::Horizontal),
             row.column_name);
  }

  auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  auto editable_flags = uneditable_flags | Qt::ItemIsEditable;

  for (const auto &row : flag_rows) {
    auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    QCOMPARE(model.index(0, row.column_number).flags(),
             row.is_editable ? editable_flags : uneditable_flags);
  }

  QCOMPARE(model.index(0, 0).data(Qt::DecorationRole), QVariant());
  QVERIFY(!(model.setData(model.index(0, 1), QVariant(), Qt::DecorationRole)));

  const auto &table_view = get_table_view(song_editor);
  for (const auto &row : get_index_pairs(model, empty_row_number,
                                         full_row_number, max_set_column)) {

    const auto &copy_index = row.first_index;
    const auto &paste_index = row.second_index;

    auto copy_value = copy_index.data();
    auto paste_value = paste_index.data();
    QCOMPARE_NE(copy_value, paste_value);

    auto &cell_editor = create_editor(table_view, paste_index);
    QCOMPARE(
        cell_editor.property(cell_editor.metaObject()->userProperty().name()),
        paste_value);

    set_editor(table_view, cell_editor, paste_index, copy_value);

    QCOMPARE(paste_index.data(), copy_value);
    undo(song_editor);
    QCOMPARE(paste_index.data(), paste_value);
  }

  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    auto delete_index = model.index(full_row_number, column_number);
    const auto &old_value = delete_index.data();

    delete_cell(song_editor, delete_index);

    QCOMPARE_NE(delete_index.data(), old_value);
    undo(song_editor);
    QCOMPARE(delete_index.data(), old_value);
  }

  auto &selector = get_selector(get_table_view(song_editor));

  for (const auto &row : get_index_pairs(model, empty_row_number,
                                         full_row_number, number_of_columns)) {
    const auto &copy_index = row.first_index;
    const auto &paste_index = row.second_index;

    auto copy_value = copy_index.data();
    auto paste_value = paste_index.data();

    QCOMPARE_NE(copy_value, paste_value);

    selector.select(copy_index, QItemSelectionModel::Select);
    trigger_copy(song_editor);
    clear_selection(selector);

    selector.select(paste_index, QItemSelectionModel::Select);
    trigger_paste_over(song_editor);
    clear_selection(selector);

    QCOMPARE(paste_index.data(), copy_value);
    undo(song_editor);
    QCOMPARE(paste_index.data(), paste_value);
  }

  for (const auto &row : get_index_pairs(model, full_row_number,
                                         empty_row_number, number_of_columns)) {
    const auto &cut_index = row.first_index;
    const auto &paste_index = row.second_index;

    const auto cut_value = cut_index.data();
    const auto paste_value = paste_index.data();
    QCOMPARE_NE(cut_value, paste_value);

    selector.select(cut_index, QItemSelectionModel::Select);
    trigger_cut(song_editor);
    clear_selection(selector);

    QCOMPARE_NE(cut_index.data(), cut_value);

    selector.select(paste_index, QItemSelectionModel::Select);
    trigger_paste_over(song_editor);
    clear_selection(selector);

    QCOMPARE(paste_index.data(), cut_value);
    undo(song_editor);
    QCOMPARE(paste_index.data(), paste_value);
    undo(song_editor);
    QCOMPARE(cut_index.data(), cut_value);
  }

  selector.select(model.index(0, 0), QItemSelectionModel::Select);
  trigger_copy(song_editor);
  clear_selection(selector);

  auto number_of_rows = model.rowCount();
  trigger_paste_into(song_editor);
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo(song_editor);
  QCOMPARE(model.rowCount(), number_of_rows);

  selector.select(model.index(0, 0), QItemSelectionModel::Select);
  trigger_paste_after(song_editor);
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo(song_editor);
  QCOMPARE(model.rowCount(), number_of_rows);
  clear_selection(selector);

  auto chord_index = model.index(0, chord_interval_column);
  auto old_child_row_count = model.rowCount(chord_index);
  selector.select(chord_index, QItemSelectionModel::Select);
  trigger_insert_into(song_editor);
  clear_selection(selector);

  QCOMPARE(model.rowCount(chord_index), old_child_row_count + 1);
  undo(song_editor);
  QCOMPARE(model.rowCount(chord_index), old_child_row_count);

  auto index = model.index(0, 0);
  auto old_row_count = model.rowCount();

  selector.select(index, QItemSelectionModel::Select);
  trigger_insert_after(song_editor);
  clear_selection(selector);

  QCOMPARE(model.rowCount(), old_row_count + 1);
  undo(song_editor);
  QCOMPARE(model.rowCount(), old_row_count);

  selector.select(model.index(0, 0), QItemSelectionModel::Select);
  trigger_remove_rows(song_editor);
  clear_selection(selector);

  QCOMPARE(model.rowCount(), old_row_count - 1);
  undo(song_editor);
  QCOMPARE(model.rowCount(), old_row_count);

  for (const auto &row : play_rows) {
    auto &selector = get_selector(get_table_view(song_editor));

    selector.select(QItemSelection(row.first_index, row.second_index),
                    QItemSelectionModel::Select);
    trigger_play(song_editor);
    // first cut off early
    trigger_play(song_editor);
    // now play for a while
    QThread::msleep(WAIT_TIME);
    trigger_stop_playing(song_editor);

    clear_selection(selector);
  }

  auto bad_paste_index = model.index(0, 0);

  for (const auto &row : bad_paste_rows) {
    auto *new_data_pointer = // NOLINT(cppcoreguidelines-owning-memory)
        new QMimeData;

    Q_ASSERT(new_data_pointer != nullptr);
    new_data_pointer->setData(row.mime_type, row.copied.toStdString().c_str());

    auto *clipboard_pointer = QGuiApplication::clipboard();
    Q_ASSERT(clipboard_pointer != nullptr);
    clipboard_pointer->setMimeData(new_data_pointer);

    selector.select(bad_paste_index, QItemSelectionModel::Select);
    close_message_later(tester, row.error_message);
    trigger_paste_over(song_editor);
    clear_selection(selector);
  }
};

void Tester::run_tests() {
  set_up();

  auto &song_editor = make_song_editor();
  open_text(song_editor, SONG_TEXT);

  auto &chords_model = get_chords_model(song_editor);
  auto &pitched_notes_model = get_pitched_notes_model(song_editor);
  auto &unpitched_notes_model = get_unpitched_notes_model(song_editor);

  for (const auto &row : std::vector({
           ToStringRow({chords_model.index(0, chord_instrument_column), ""}),
           ToStringRow(
               {chords_model.index(0, chord_percussion_set_column), ""}),
           ToStringRow(
               {chords_model.index(0, chord_percussion_instrument_column), ""}),
           ToStringRow({chords_model.index(0, chord_interval_column), ""}),
           ToStringRow({chords_model.index(1, chord_interval_column), "3"}),
           ToStringRow({chords_model.index(2, chord_interval_column), "/2"}),
           ToStringRow({chords_model.index(3, chord_interval_column), "3/2"}),
           ToStringRow({chords_model.index(4, chord_interval_column), "o1"}),
           ToStringRow({chords_model.index(5, chord_interval_column), "3o1"}),
           ToStringRow({chords_model.index(6, chord_interval_column), "/2o1"}),
           ToStringRow({chords_model.index(7, chord_interval_column), "3/2o1"}),
           ToStringRow({chords_model.index(0, chord_beats_column), ""}),
           ToStringRow({chords_model.index(1, chord_beats_column), "3"}),
           ToStringRow({chords_model.index(2, chord_beats_column), "/2"}),
           ToStringRow({chords_model.index(3, chord_beats_column), "3/2"}),
       })) {
    QCOMPARE(row.index.data().toString(), row.text);
  }

  QCOMPARE(chords_model.rowCount(), 8);

  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 8}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    trigger_edit_pitched_notes(song_editor, row.chord_number);
    QCOMPARE(pitched_notes_model.rowCount(), row.number);
    undo(song_editor);
  }

  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 4}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    trigger_edit_unpitched_notes(song_editor, row.chord_number);
    QCOMPARE(unpitched_notes_model.rowCount(), row.number);
    undo(song_editor);
  }

  trigger_edit_unpitched_notes(song_editor, 0);
  trigger_back_to_chords(song_editor);
  undo(song_editor);
  undo(song_editor);

  test_number_of_columns(chords_model, number_of_chord_columns);
  test_number_of_columns(pitched_notes_model, number_of_pitched_note_columns);
  test_number_of_columns(unpitched_notes_model,
                         number_of_unpitched_note_columns);

  auto old_gain = get_gain(song_editor);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  set_gain(song_editor, NEW_GAIN_1);
  QCOMPARE(get_gain(song_editor), NEW_GAIN_1);
  set_gain(song_editor, NEW_GAIN_2);
  QCOMPARE(get_gain(song_editor), NEW_GAIN_2);

  undo(song_editor);
  QCOMPARE(get_gain(song_editor), old_gain);

  auto old_key = get_starting_key(song_editor);
  QCOMPARE_NE(old_key, STARTING_KEY_1);
  QCOMPARE_NE(old_key, STARTING_KEY_2);

  // test combining
  set_starting_key(song_editor, STARTING_KEY_1);
  QCOMPARE(get_starting_key(song_editor), STARTING_KEY_1);
  set_starting_key(song_editor, STARTING_KEY_2);
  QCOMPARE(get_starting_key(song_editor), STARTING_KEY_2);
  undo(song_editor);
  QCOMPARE(get_starting_key(song_editor), old_key);

  auto old_velocity = get_starting_velocity(song_editor);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_1);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_2);

  // test combining
  set_starting_velocity(song_editor, STARTING_VELOCITY_1);
  QCOMPARE(get_starting_velocity(song_editor), STARTING_VELOCITY_1);
  set_starting_velocity(song_editor, STARTING_VELOCITY_2);
  QCOMPARE(get_starting_velocity(song_editor), STARTING_VELOCITY_2);
  undo(song_editor);
  QCOMPARE(get_starting_velocity(song_editor), old_velocity);

  auto old_tempo = get_starting_tempo(song_editor);

  // test combining
  set_starting_tempo(song_editor, STARTING_TEMPO_1);
  QCOMPARE(get_starting_tempo(song_editor), STARTING_TEMPO_1);
  set_starting_tempo(song_editor, STARTING_TEMPO_2);
  QCOMPARE(get_starting_tempo(song_editor), STARTING_TEMPO_2);
  undo(song_editor);
  QCOMPARE(get_starting_tempo(song_editor), old_tempo);

  QCOMPARE(chords_model.headerData(0, Qt::Vertical), QVariant(1));
  QCOMPARE(chords_model.headerData(0, Qt::Vertical, Qt::DecorationRole),
           QVariant());

  for (const auto &row : std::vector({
           FrequencyRow({A_MINUS_FREQUENCY, "217 Hz; A3 − 24 cents"}),
           FrequencyRow({A_PLUS_FREQUENCY, "223 Hz; A3 + 23 cents"}),
           FrequencyRow({A_FREQUENCY, "220 Hz; A3"}),
           FrequencyRow({B_FLAT_FREQUENCY, "233 Hz; B♭3 − 1 cents"}),
           FrequencyRow({B_FREQUENCY, "247 Hz; B3"}),
           FrequencyRow({C_FREQUENCY, "262 Hz; C4 + 2 cents"}),
           FrequencyRow({C_SHARP_FREQUENCY, "277 Hz; C♯4 − 1 cents"}),
           FrequencyRow({D_FREQUENCY, "294 Hz; D4 + 2 cents"}),
           FrequencyRow({E_FLAT_FREQUENCY, "311 Hz; E♭4 − 1 cents"}),
           FrequencyRow({E_FREQUENCY, "330 Hz; E4 + 2 cents"}),
           FrequencyRow({F_FREQUENCY, "349 Hz; F4 − 1 cents"}),
           FrequencyRow({F_SHARP_FREQUENCY, "370 Hz; F♯4"}),
           FrequencyRow({G_FREQUENCY, "392 Hz; G4"}),
           FrequencyRow({A_FLAT_FREQUENCY, "415 Hz; A♭4 − 1 cents"}),
       })) {
    set_starting_key(song_editor, row.frequency);
    QCOMPARE(
        chords_model.index(0, chord_interval_column).data(Qt::StatusTipRole),
        row.text);
    undo(song_editor);
  }

  trigger_edit_pitched_notes(song_editor, 1);
  set_starting_key(song_editor, A_FREQUENCY);
  QCOMPARE(pitched_notes_model.index(0, chord_interval_column)
               .data(Qt::StatusTipRole),
           "660 Hz; E5 + 2 cents");
  undo(song_editor);
  undo(song_editor);

  trigger_edit_unpitched_notes(song_editor, 1);
  QCOMPARE(unpitched_notes_model.index(0, chord_interval_column)
               .data(Qt::StatusTipRole),
           "");
  undo(song_editor);

  test_model(
      *this, song_editor, chords_model,
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
           HeaderRow({chord_unpitched_notes_column, "Unpitched notes"})}),
      std::vector({FlagRow({chord_interval_column, true}),
                   FlagRow({chord_pitched_notes_column, false}),
                   FlagRow({chord_unpitched_notes_column, false})}),
      std::vector({
          TwoIndicesRow({chords_model.index(0, chord_interval_column),
                         chords_model.index(1, chord_interval_column)}),
          TwoIndicesRow({chords_model.index(1, chord_interval_column),
                         chords_model.index(1, chord_interval_column)}),
      }),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime as "
                        "chords cells"}),
           BadPasteRow(
               {"", PITCHED_NOTES_CELLS_MIME,
                "Cannot paste pitched notes cells as "
                "chords cells"}),
           BadPasteRow(
               {"[", CHORDS_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", CHORDS_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", CHORDS_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}),
      0, 1, number_of_chord_columns, number_of_chord_columns - 2);

  trigger_edit_pitched_notes(song_editor, 1);
  test_model(
      *this, song_editor, pitched_notes_model,
      std::vector(
          {HeaderRow({pitched_note_instrument_column, "Instrument"}),
           HeaderRow({pitched_note_interval_column, "Interval"}),
           HeaderRow({pitched_note_beats_column, "Beats"}),
           HeaderRow({pitched_note_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({pitched_note_words_column, "Words"})}),
      std::vector({FlagRow({pitched_note_interval_column, true})}),
      std::vector({
          TwoIndicesRow(
              {pitched_notes_model.index(0, pitched_note_interval_column),
               pitched_notes_model.index(1, pitched_note_interval_column)}),
          TwoIndicesRow(
              {pitched_notes_model.index(1, pitched_note_interval_column),
               pitched_notes_model.index(1, pitched_note_interval_column)}),
      }),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime as "
                        "pitched notes cells"}),
           BadPasteRow({"", CHORDS_CELLS_MIME,
                        "Cannot paste chords cells as "
                        "pitched notes cells"}),
           BadPasteRow(
               {"[", PITCHED_NOTES_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", PITCHED_NOTES_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", PITCHED_NOTES_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}),
      0, 1, number_of_pitched_note_columns, number_of_pitched_note_columns);
  undo(song_editor);

  trigger_edit_unpitched_notes(song_editor, 1);
  test_model(
      *this, song_editor, unpitched_notes_model,
      std::vector(
          {HeaderRow({unpitched_note_percussion_set_column, "Percussion set"}),
           HeaderRow({unpitched_note_percussion_instrument_column,
                      "Percussion instrument"}),
           HeaderRow({unpitched_note_beats_column, "Beats"}),
           HeaderRow({unpitched_note_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({unpitched_note_words_column, "Words"})}),
      std::vector({FlagRow({unpitched_note_percussion_set_column, true})}),
      std::vector({
          TwoIndicesRow({unpitched_notes_model.index(
                             0, unpitched_note_percussion_set_column),
                         unpitched_notes_model.index(
                             1, unpitched_note_percussion_set_column)}),
          TwoIndicesRow({unpitched_notes_model.index(
                             1, unpitched_note_percussion_set_column),
                         unpitched_notes_model.index(
                             1, unpitched_note_percussion_set_column)}),
      }),
      std::vector(
          {BadPasteRow({"", "not a mime",
                        "Cannot paste not a mime as "
                        "unpitched notes cells"}),
           BadPasteRow({"", CHORDS_CELLS_MIME,
                        "Cannot paste chords cells as "
                        "unpitched notes cells"}),
           BadPasteRow(
               {"[", UNPITCHED_NOTES_CELLS_MIME,
                "[json.exception.parse_error.101] parse error at line 1, "
                "column 2: syntax error while parsing value - unexpected end "
                "of input; expected '[', '{', or a literal"}),
           BadPasteRow({"[]", UNPITCHED_NOTES_CELLS_MIME, "Nothing to paste!"}),
           BadPasteRow({"{\"a\": 1}", UNPITCHED_NOTES_CELLS_MIME,
                        "At  of {\"a\":1} - required property 'left_column' "
                        "not found in object\n"})}),
      0, 1, number_of_unpitched_note_columns, number_of_unpitched_note_columns);
  undo(song_editor);

  trigger_edit_pitched_notes(song_editor, 1);

  set_starting_velocity(song_editor, BIG_VELOCITY);

  close_message_later(
      *this, "Velocity 378 exceeds 127 for chord 2, pitched note 1. Playing "
             "with velocity 127");

  play_cell(song_editor,
            pitched_notes_model.index(0, pitched_note_interval_column));

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor);
  undo(song_editor);

  undo(song_editor);

  trigger_edit_pitched_notes(song_editor, 2);

  for (auto number = 0; number < OVERLOAD_NUMBER; number = number + 1) {
    trigger_insert_into(song_editor);
  }

  trigger_back_to_chords(song_editor);

  close_message_later(*this,
                      "Out of MIDI channels for chord 3, pitched note 17. Not "
                      "playing note.");

  play_cell(song_editor, chords_model.index(2, chord_interval_column));

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_editor);

  // undo back to chords
  undo(song_editor);

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    undo(song_editor);
  }

  // undo edit pitched_notes
  undo(song_editor);

  delete_cell(song_editor, chords_model.index(1, chord_instrument_column));
  delete_cell(song_editor, chords_model.index(1, chord_percussion_set_column));
  delete_cell(song_editor,
              chords_model.index(1, chord_percussion_instrument_column));

  trigger_edit_pitched_notes(song_editor, 1);

  delete_cell(song_editor,
              pitched_notes_model.index(0, pitched_note_instrument_column));
  QCOMPARE(
      pitched_notes_model
          .data(pitched_notes_model.index(0, pitched_note_instrument_column))
          .toString(),
      "");
  close_message_later(
      *this, "No instrument for chord 2, pitched note 1. Using Marimba.");

  play_cell(song_editor,
            pitched_notes_model.index(0, pitched_note_instrument_column));

  // undo delete pitched_note instrument
  undo(song_editor);
  // undo edit pitched_notes
  undo(song_editor);

  trigger_edit_unpitched_notes(song_editor, 1);

  delete_cell(song_editor, unpitched_notes_model.index(
                               0, unpitched_note_percussion_set_column));

  close_message_later(
      *this,
      "No percussion set for chord 2, unpitched note 1. Using Standard.");

  play_cell(song_editor, unpitched_notes_model.index(
                             0, unpitched_note_percussion_set_column));
  // undo edit delete unpitched_note set
  undo(song_editor);

  delete_cell(song_editor, unpitched_notes_model.index(
                               0, unpitched_note_percussion_instrument_column));

  close_message_later(*this, "No percussion instrument for chord 2, "
                             "unpitched note 1. Using Tambourine.");

  play_cell(song_editor, unpitched_notes_model.index(
                             0, unpitched_note_percussion_set_column));
  // undo delete unpitched_note instrument
  undo(song_editor);
  // undo edit unpitched_notes
  undo(song_editor);
  // undo delete chord unpitched_note instrument
  undo(song_editor);
  // undo delete chord unpitched_note set
  undo(song_editor);
  // undo delete chord instrument
  undo(song_editor);

  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  auto file_name = temp_json_file.fileName();
  save_as_file(song_editor, file_name);

  QCOMPARE(get_current_file(song_editor), file_name);

  QVERIFY(temp_json_file.open());
  auto written = QString(temp_json_file.readAll());
  temp_json_file.close();
// different encoding on windows or something
#ifndef _WIN32
  QCOMPARE(written, SONG_TEXT);
#endif
  trigger_save(song_editor);

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(song_editor, temp_export_file.fileName());

  for (const auto &row : std::vector({
           TwoStringsRow(
               {"{", "[json.exception.parse_error.101] parse error at line 1, "
                     "column 2: syntax error while parsing object key - "
                     "unexpected end of input; expected string literal"}),
           TwoStringsRow({"[]", "At  of [] - unexpected instance type\n"}),
           TwoStringsRow({"[1]", "At  of [1] - unexpected instance type\n"}),
       })) {
    close_message_later(*this, row.second_string);
    open_text(song_editor, row.first_string);
  }

  open_text(song_editor, R""""({
    "gain": 5.0,
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_velocity": 64
})"""");
  delete_song_editor(song_editor);
};

QTEST_MAIN(Tester);

#include "test.moc"