#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QItemSelectionModel>
#include <QtCore/QList>
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
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>
#include <string>
#include <vector>

// IWYU pragma: no_include <algorithm>
// IWYU pragma: no_include <utility>

#include "justly/justly.hpp"

struct SongWidget;
struct SongMenuBar;

static const auto BIG_VELOCITY = 126;
static const auto PERCUSSION_ROWS = 16;
static const auto MUSIC_XML_ROWS = 545;
static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;
static const auto SELECT_AND_CLEAR =
    QItemSelectionModel::Select | QItemSelectionModel::Clear;
static const auto SHIFT_TIMES = 20;
static const auto STARTING_KEY_1 = 401.0;
static const auto STARTING_KEY_2 = 402.0;
static const auto STARTING_TEMPO_1 = 150.0;
static const auto STARTING_TEMPO_2 = 100.0;
static const auto STARTING_VELOCITY_1 = 70;
static const auto STARTING_VELOCITY_2 = 80;
static const auto WAIT_TIME = 500;

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

template <typename Thing>
[[nodiscard]] static auto get_reference(Thing *thing_pointer) -> Thing & {
  Q_ASSERT(thing_pointer != nullptr);
  return *thing_pointer;
}

static void double_click_column(QAbstractItemView &table,
                                const int chord_number,
                                const int chord_column) {
  table.doubleClicked(
      get_reference(table.model()).index(chord_number, chord_column));
}

static void set_with_editor(const QAbstractItemView &table_view,
                            QModelIndex index, const QVariant &new_value) {
  auto &delegate = get_reference(table_view.itemDelegate());
  auto &cell_editor = get_reference(delegate.createEditor(
      &get_reference(table_view.viewport()), QStyleOptionViewItem(), index));
  delegate.setEditorData(&cell_editor, index);
  cell_editor.setProperty(
      get_reference(cell_editor.metaObject()).userProperty().name(), new_value);
  delegate.setModelData(&cell_editor, table_view.model(), index);
}

[[nodiscard]] static auto get_index_pairs(QAbstractItemModel &model,
                                          const int first_row_number,
                                          const int second_row_number,
                                          const int min_column = 0) {
  std::vector<TwoIndicesRow> rows;
  const auto number_of_columns = model.columnCount();
  for (auto column_number = min_column; column_number < number_of_columns;
       column_number = column_number + 1) {
    Q_ASSERT(model.hasIndex(first_row_number, column_number));
    Q_ASSERT(model.hasIndex(second_row_number, column_number));
    rows.push_back({model.index(first_row_number, column_number),
                    model.index(second_row_number, column_number)});
  }
  return rows;
}

static void delete_cell(SongMenuBar &song_menu_bar,
                        QItemSelectionModel &selector,
                        const QModelIndex &index) {
  selector.select(index, SELECT_AND_CLEAR);
  Q_ASSERT(selector.selection().size() == 1);
  trigger_delete_cells(song_menu_bar);
}

static void play_cell(SongMenuBar &song_menu_bar, QItemSelectionModel &selector,
                      const QModelIndex &index) {
  selector.select(index, SELECT_AND_CLEAR);
  trigger_play(song_menu_bar);
}

static void open_text(SongWidget &song_widget, const QString &json_song) {
  QTemporaryFile json_file;
  QVERIFY(json_file.open());
  json_file.write(json_song.toStdString().c_str());
  json_file.close();
  open_file(song_widget, json_file.fileName());
}

static void test_number_of_columns(const QAbstractItemModel &model,
                                   const int number_of_columns) {
  QCOMPARE(model.columnCount(), number_of_columns);
}

static void test_previous_next_chord(SongMenuBar &song_menu_bar,
                                     SongWidget &song_widget,
                                     const int chord_number) {
  QCOMPARE(get_current_chord_number(song_widget), chord_number);
  trigger_previous_chord(song_menu_bar);
  QCOMPARE(get_current_chord_number(song_widget), chord_number - 1);
  trigger_next_chord(song_menu_bar);
  QCOMPARE(get_current_chord_number(song_widget), chord_number);
}

struct Tester : public QObject {
  Q_OBJECT

public:
  QString test_folder;
  bool waiting_for_message = false;

private slots:
  void run_tests();
};

static void close_message_later(Tester &tester, const QString &expected_text) {
  const auto waiting_before = tester.waiting_for_message;
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

static void test_new_value(SongWidget &song_widget, const QModelIndex &index,
                           const QVariant &old_value) {
  QCOMPARE_NE(old_value, index.data());
  undo(song_widget);
  QCOMPARE(old_value, index.data());
}

static void test_model(Tester &tester, SongEditor &song_editor,
                       QAbstractItemView &table,
                       const std::vector<HeaderRow> &column_header_rows,
                       const std::vector<FlagRow> &flag_rows,
                       const std::vector<TwoIndicesRow> &play_rows,
                       const std::vector<BadPasteRow> &bad_paste_rows,
                       const int empty_row_number, const int full_row_number,
                       const int min_set_column = 0) {
  auto &model = *(table.model());
  auto &selector = *(table.selectionModel());

  auto &song_widget = song_editor.song_widget;
  auto &song_menu_bar = song_editor.song_menu_bar;
  for (const auto &row : column_header_rows) {
    QCOMPARE(model.headerData(row.column_number, Qt::Horizontal),
             row.column_name);
  }

  const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  const auto editable_flags = uneditable_flags | Qt::ItemIsEditable;

  for (const auto &row : flag_rows) {
    const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    QCOMPARE(model.index(0, row.column_number).flags(),
             row.is_editable ? editable_flags : uneditable_flags);
  }

  QCOMPARE(model.index(0, 0).data(Qt::DecorationRole), QVariant());
  QVERIFY(!(model.setData(model.index(0, 1), QVariant(), Qt::DecorationRole)));

  for (const auto &row : get_index_pairs(model, empty_row_number,
                                         full_row_number, min_set_column)) {

    const auto &empty_index = row.first_index;
    const auto &full_index = row.second_index;

    const auto empty_value = empty_index.data();
    const auto full_value = full_index.data();
    QCOMPARE_NE(empty_value, full_value);

    set_with_editor(table, full_index, empty_value);
    QCOMPARE(full_index.data(), empty_value);
    undo(song_widget);
    QCOMPARE(full_index.data(), full_value);
  }

  const auto number_of_columns = model.columnCount();
  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    const auto delete_index = model.index(full_row_number, column_number);
    const auto &old_value = delete_index.data();

    delete_cell(song_menu_bar, selector, delete_index);
    test_new_value(song_widget, delete_index, old_value);
  }

  for (const auto &row :
       get_index_pairs(model, empty_row_number, full_row_number)) {
    const auto &empty_index = row.first_index;
    const auto &full_index = row.second_index;

    const auto empty_value = empty_index.data();
    const auto full_value = full_index.data();

    QCOMPARE_NE(empty_value, full_value);

    selector.select(empty_index, SELECT_AND_CLEAR);
    trigger_copy(song_menu_bar);

    selector.select(full_index, SELECT_AND_CLEAR);
    trigger_paste_over(song_menu_bar);

    QCOMPARE(full_index.data(), empty_value);
    undo(song_widget);
    QCOMPARE(full_index.data(), full_value);

    selector.select(full_index, SELECT_AND_CLEAR);
    trigger_cut(song_menu_bar);

    QCOMPARE(full_index.data(), empty_value);

    selector.select(empty_index, SELECT_AND_CLEAR);
    trigger_paste_over(song_menu_bar);

    QCOMPARE(empty_index.data(), full_value);
    undo(song_widget);
    QCOMPARE(empty_index.data(), empty_value);
    undo(song_widget);
    QCOMPARE(full_index.data(), full_value);
  }

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  trigger_copy(song_menu_bar);

  const auto number_of_rows = model.rowCount();
  trigger_paste_into(song_menu_bar);
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo(song_widget);
  QCOMPARE(model.rowCount(), number_of_rows);

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  trigger_paste_after(song_menu_bar);
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo(song_widget);
  QCOMPARE(model.rowCount(), number_of_rows);

  const auto chord_index = model.index(0, chord_interval_column);
  const auto old_child_row_count = model.rowCount(chord_index);
  selector.select(chord_index, SELECT_AND_CLEAR);
  trigger_insert_into(song_menu_bar);

  QCOMPARE(model.rowCount(chord_index), old_child_row_count + 1);
  undo(song_widget);
  QCOMPARE(model.rowCount(chord_index), old_child_row_count);

  const auto index = model.index(0, 0);
  const auto old_row_count = model.rowCount();

  selector.select(index, SELECT_AND_CLEAR);
  trigger_insert_after(song_menu_bar);

  QCOMPARE(model.rowCount(), old_row_count + 1);
  undo(song_widget);
  QCOMPARE(model.rowCount(), old_row_count);

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  trigger_remove_rows(song_menu_bar);

  QCOMPARE(model.rowCount(), old_row_count - 1);
  undo(song_widget);
  QCOMPARE(model.rowCount(), old_row_count);

  for (const auto &row : play_rows) {
    selector.select(QItemSelection(row.first_index, row.second_index),
                    SELECT_AND_CLEAR);
    trigger_play(song_menu_bar);
    // first cut off early
    trigger_play(song_menu_bar);
    // now play for a while
    QThread::msleep(WAIT_TIME);
    trigger_stop_playing(song_menu_bar);
  }

  auto bad_paste_index = model.index(0, 0);

  for (const auto &row : bad_paste_rows) {
    auto *const new_data_pointer = // NOLINT(cppcoreguidelines-owning-memory)
        new QMimeData;

    Q_ASSERT(new_data_pointer != nullptr);
    new_data_pointer->setData(row.mime_type, row.copied.toStdString().c_str());

    auto *const clipboard_pointer = QGuiApplication::clipboard();
    Q_ASSERT(clipboard_pointer != nullptr);
    clipboard_pointer->setMimeData(new_data_pointer);

    selector.select(bad_paste_index, SELECT_AND_CLEAR);
    close_message_later(tester, row.error_message);
    trigger_paste_over(song_menu_bar);
  }
};

static void test_buttons(SongWidget &song_widget, QAbstractItemView &table,
                         int interval_column) {
  const auto &model = get_reference(table.model());
  auto &selector = get_reference(table.selectionModel());
  const auto first_index = model.index(0, interval_column);
  const auto original_data = first_index.data();
  selector.select(first_index, SELECT_AND_CLEAR);

  trigger_third_down(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_third_up(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_fifth_down(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_fifth_up(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_seventh_down(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_seventh_up(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_octave_down(song_widget);
  test_new_value(song_widget, first_index, original_data);

  trigger_octave_up(song_widget);
  test_new_value(song_widget, first_index, original_data);
}

void Tester::run_tests() {
  set_up();

  SongEditor song_editor;

  auto &song_widget = song_editor.song_widget;
  auto &song_menu_bar = song_editor.song_menu_bar;

  QDir test_dir(test_folder);

  const auto test_song_file = test_dir.filePath("test_song.json");

  open_file(song_widget, test_song_file);

  auto &chords_table = get_chords_table(song_widget);
  auto &pitched_notes_table = get_pitched_notes_table(song_widget);
  auto &unpitched_notes_table = get_unpitched_notes_table(song_widget);

  auto &chords_model = *(chords_table.model());
  auto &pitched_notes_model = *(pitched_notes_table.model());
  auto &unpitched_notes_model = *(unpitched_notes_table.model());

  auto &chords_selector = *(chords_table.selectionModel());
  auto &pitched_notes_selector = *(pitched_notes_table.selectionModel());
  auto &unpitched_notes_selector = *(unpitched_notes_table.selectionModel());

  // test buttons
  test_buttons(song_widget, chords_table, chord_interval_column);

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  test_buttons(song_widget, pitched_notes_table, pitched_note_interval_column);
  undo(song_widget); // back to chords

  for (const auto &row : std::vector({
           ToStringRow({chords_model.index(0, chord_instrument_column), ""}),
           ToStringRow(
               {chords_model.index(0, chord_percussion_instrument_column), ""}),
           ToStringRow({chords_model.index(0, chord_interval_column), ""}),
           ToStringRow({chords_model.index(1, chord_interval_column), "3"}),
           ToStringRow({chords_model.index(2, chord_interval_column), "/5"}),
           ToStringRow({chords_model.index(3, chord_interval_column), "3/5"}),
           ToStringRow({chords_model.index(4, chord_interval_column), "o1"}),
           ToStringRow({chords_model.index(5, chord_interval_column), "3o1"}),
           ToStringRow({chords_model.index(6, chord_interval_column), "/5o1"}),
           ToStringRow({chords_model.index(7, chord_interval_column), "3/5o1"}),
           ToStringRow({chords_model.index(0, chord_beats_column), ""}),
           ToStringRow({chords_model.index(1, chord_beats_column), "3"}),
           ToStringRow({chords_model.index(2, chord_beats_column), "/5"}),
           ToStringRow({chords_model.index(3, chord_beats_column), "3/5"}),
           ToStringRow(
               {chords_model.index(1, chord_percussion_instrument_column),
                "Standard #35"}),
       })) {
    QCOMPARE(row.index.data().toString(), row.text);
  }

  QCOMPARE(chords_model.rowCount(), 8);

  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 8}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    double_click_column(chords_table, row.chord_number,
                        chord_pitched_notes_column);
    QCOMPARE(pitched_notes_model.rowCount(), row.number);
    undo(song_widget);
  }

  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 4}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    double_click_column(chords_table, row.chord_number,
                        chord_unpitched_notes_column);
    QCOMPARE(unpitched_notes_model.rowCount(), row.number);
    undo(song_widget);
  }

  double_click_column(chords_table, 0, chord_unpitched_notes_column);
  trigger_back_to_chords(song_menu_bar);
  undo(song_widget);
  undo(song_widget);

  test_number_of_columns(chords_model, number_of_chord_columns);
  test_number_of_columns(pitched_notes_model, number_of_pitched_note_columns);
  test_number_of_columns(unpitched_notes_model,
                         number_of_unpitched_note_columns);

  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  set_gain(song_widget, NEW_GAIN_1);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
  set_gain(song_widget, NEW_GAIN_2);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_2);

  undo(song_widget);
  QCOMPARE(get_gain(song_widget), old_gain);

  const auto old_key = get_starting_key(song_widget);
  QCOMPARE_NE(old_key, STARTING_KEY_1);
  QCOMPARE_NE(old_key, STARTING_KEY_2);

  // test combining
  set_starting_key(song_widget, STARTING_KEY_1);
  QCOMPARE(get_starting_key(song_widget), STARTING_KEY_1);
  set_starting_key(song_widget, STARTING_KEY_2);
  QCOMPARE(get_starting_key(song_widget), STARTING_KEY_2);
  undo(song_widget);
  QCOMPARE(get_starting_key(song_widget), old_key);

  const auto old_velocity = get_starting_velocity(song_widget);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_1);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_2);

  // test combining
  set_starting_velocity(song_widget, STARTING_VELOCITY_1);
  QCOMPARE(get_starting_velocity(song_widget), STARTING_VELOCITY_1);
  set_starting_velocity(song_widget, STARTING_VELOCITY_2);
  QCOMPARE(get_starting_velocity(song_widget), STARTING_VELOCITY_2);
  undo(song_widget);
  QCOMPARE(get_starting_velocity(song_widget), old_velocity);

  const auto old_tempo = get_starting_tempo(song_widget);

  // test combining
  set_starting_tempo(song_widget, STARTING_TEMPO_1);
  QCOMPARE(get_starting_tempo(song_widget), STARTING_TEMPO_1);
  set_starting_tempo(song_widget, STARTING_TEMPO_2);
  QCOMPARE(get_starting_tempo(song_widget), STARTING_TEMPO_2);
  undo(song_widget);
  QCOMPARE(get_starting_tempo(song_widget), old_tempo);

  QCOMPARE(chords_model.headerData(0, Qt::Vertical), QVariant(1));
  QCOMPARE(chords_model.headerData(0, Qt::Vertical, Qt::DecorationRole),
           QVariant());

  for (const auto &row : std::vector({
           FrequencyRow(
               {A_MINUS_FREQUENCY,
                "217 Hz ≈ A3 − 24 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {A_PLUS_FREQUENCY,
                "223 Hz ≈ A3 + 23 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow({A_FREQUENCY,
                         "220 Hz ≈ A3; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {B_FLAT_FREQUENCY,
                "233 Hz ≈ B♭3 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow({B_FREQUENCY,
                         "247 Hz ≈ B3; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {C_FREQUENCY,
                "262 Hz ≈ C4 + 2 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {C_SHARP_FREQUENCY,
                "277 Hz ≈ C♯4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {D_FREQUENCY,
                "294 Hz ≈ D4 + 2 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {E_FLAT_FREQUENCY,
                "311 Hz ≈ E♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {E_FREQUENCY,
                "330 Hz ≈ E4 + 2 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {F_FREQUENCY,
                "349 Hz ≈ F4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow({F_SHARP_FREQUENCY,
                         "370 Hz ≈ F♯4; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow({G_FREQUENCY,
                         "392 Hz ≈ G4; Velocity 10; 100 bpm; Start at 0 ms"}),
           FrequencyRow(
               {A_FLAT_FREQUENCY,
                "415 Hz ≈ A♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms"}),
       })) {
    set_starting_key(song_widget, row.frequency);
    QCOMPARE(
        chords_model.index(0, chord_interval_column).data(Qt::StatusTipRole),
        row.text);
    undo(song_widget);
  }

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  set_starting_key(song_widget, A_FREQUENCY);
  QCOMPARE(pitched_notes_model.index(0, pitched_note_interval_column)
               .data(Qt::StatusTipRole),
           "660 Hz ≈ E5 + 2 cents; Velocity 30; 300 bpm; Start at 600 ms");
  undo(song_widget);
  undo(song_widget);

  double_click_column(chords_table, 1, chord_unpitched_notes_column);
  QCOMPARE(unpitched_notes_model.index(0, 0).data(Qt::StatusTipRole), "");
  undo(song_widget);

  test_model(
      *this, song_editor, chords_table,
      std::vector(
          {HeaderRow({chord_instrument_column, "Instrument"}),
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
           BadPasteRow({"", PITCHED_NOTES_CELLS_MIME,
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
      0, 1, chord_instrument_column);

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  test_model(
      *this, song_editor, pitched_notes_table,
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
      0, 1);
  undo(song_widget);

  double_click_column(chords_table, 1, chord_unpitched_notes_column);
  test_model(
      *this, song_editor, unpitched_notes_table,
      std::vector(
          {HeaderRow({unpitched_note_percussion_instrument_column,
                      "Percussion instrument"}),
           HeaderRow({unpitched_note_beats_column, "Beats"}),
           HeaderRow({unpitched_note_velocity_ratio_column, "Velocity ratio"}),
           HeaderRow({unpitched_note_words_column, "Words"})}),
      std::vector(
          {FlagRow({unpitched_note_percussion_instrument_column, true})}),
      std::vector({
          TwoIndicesRow({unpitched_notes_model.index(
                             0, unpitched_note_percussion_instrument_column),
                         unpitched_notes_model.index(
                             1, unpitched_note_percussion_instrument_column)}),
          TwoIndicesRow({unpitched_notes_model.index(
                             1, unpitched_note_percussion_instrument_column),
                         unpitched_notes_model.index(
                             1, unpitched_note_percussion_instrument_column)}),
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
      0, 1);
  undo(song_widget); // back to chords

  double_click_column(chords_table, 1, chord_pitched_notes_column);

  set_starting_velocity(song_widget, BIG_VELOCITY);

  close_message_later(*this,
                      "Velocity 378 exceeds 127 for chord 2, pitched note 1");

  play_cell(song_menu_bar, pitched_notes_selector,
            pitched_notes_model.index(0, pitched_note_interval_column));

  QThread::msleep(WAIT_TIME);
  trigger_stop_playing(song_menu_bar);
  undo(song_widget); // undo set starting velocity

  // HERE
  close_message_later(*this,
                      "Frequency 6.9206e+08 for chord 2, pitched note 1 "
                      "greater than or equal to maximum frequency 12911.4");
  pitched_notes_selector.select(
      pitched_notes_model.index(0, pitched_note_interval_column),
      SELECT_AND_CLEAR);
  for (auto counter = 0; counter < SHIFT_TIMES; counter++) {
    trigger_octave_up(song_widget);
  }
  play_cell(song_menu_bar, pitched_notes_selector,
            pitched_notes_model.index(0, pitched_note_interval_column));
  for (auto counter = 0; counter < SHIFT_TIMES; counter++) {
    undo(song_widget);
  }

  close_message_later(*this, "Frequency 0.000629425 for chord 2, pitched note "
                             "1 less than minimum frequency 7.94305");
  for (auto counter = 0; counter < SHIFT_TIMES; counter++) {
    trigger_octave_down(song_widget);
  }
  play_cell(song_menu_bar, pitched_notes_selector,
            pitched_notes_model.index(0, pitched_note_interval_column));
  for (auto counter = 0; counter < SHIFT_TIMES; counter++) {
    undo(song_widget);
  }

  undo(song_widget); // undo back to chords

  delete_cell(song_menu_bar, chords_selector,
              chords_model.index(1, chord_instrument_column));

  double_click_column(chords_table, 1, chord_pitched_notes_column);

  const auto instrument_delete_index =
      pitched_notes_model.index(1, pitched_note_instrument_column);

  delete_cell(song_menu_bar, pitched_notes_selector, instrument_delete_index);
  QCOMPARE(instrument_delete_index.data().toString(), "");
  close_message_later(*this, "No instrument for chord 2, pitched note 2");

  play_cell(song_menu_bar, pitched_notes_selector, instrument_delete_index);

  // undo delete pitched_note instrument
  undo(song_widget);
  // undo edit pitched_notes
  undo(song_widget);
  // undo delete chord instrument
  undo(song_widget);

  delete_cell(song_menu_bar, chords_selector,
              chords_model.index(1, chord_percussion_instrument_column));

  double_click_column(chords_table, 1, chord_unpitched_notes_column);

  const auto percussion_instrument_delete_index = unpitched_notes_model.index(
      1, unpitched_note_percussion_instrument_column);

  delete_cell(song_menu_bar, unpitched_notes_selector,
              percussion_instrument_delete_index);

  close_message_later(*this, "No percussion set for chord 2, unpitched note 2");

  play_cell(song_menu_bar, unpitched_notes_selector,
            percussion_instrument_delete_index);
  // undo edit delete unpitched_note set
  undo(song_widget);

  delete_cell(song_menu_bar, unpitched_notes_selector,
              percussion_instrument_delete_index);

  close_message_later(*this, "No percussion set for chord 2, "
                             "unpitched note 2");

  play_cell(song_menu_bar, unpitched_notes_selector,
            percussion_instrument_delete_index);
  // undo delete unpitched_note percussion instrument
  undo(song_widget);
  // undo edit unpitched_notes
  undo(song_widget);
  // undo delete chord percussion instrument
  undo(song_widget);
  // undo delete chord percussion set
  undo(song_widget);

  double_click_column(chords_table, 1, chord_unpitched_notes_column);
  test_previous_next_chord(song_menu_bar, song_widget, 1);
  trigger_back_to_chords(song_menu_bar);

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  test_previous_next_chord(song_menu_bar, song_widget, 1);
  trigger_back_to_chords(song_menu_bar);

  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  const auto file_name = temp_json_file.fileName();
  save_as_file(song_widget, file_name);

  QCOMPARE(get_current_file(song_widget), file_name);

  QVERIFY(temp_json_file.open());
  const auto written = QString(temp_json_file.readAll());
  temp_json_file.close();

  QFile test_song_qfile(test_song_file);

  QVERIFY(test_song_qfile.open(QIODevice::ReadOnly));
  const auto original = QString(test_song_qfile.readAll());
  test_song_qfile.close();

  QCOMPARE(original, written);
  trigger_save(song_menu_bar);

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(song_widget, temp_export_file.fileName());

  for (const auto &row : std::vector({
           TwoStringsRow(
               {"{", "[json.exception.parse_error.101] parse error at line 1, "
                     "column 2: syntax error while parsing object key - "
                     "unexpected end of input; expected string literal"}),
           TwoStringsRow({"[]", "At  of [] - unexpected instance type\n"}),
           TwoStringsRow({"[1]", "At  of [1] - unexpected instance type\n"}),
       })) {
    close_message_later(*this, row.second_string);
    open_text(song_widget, row.first_string);
  }

  open_text(song_widget, R""""({
    "gain": 5.0,
    "starting_key": 220,
    "starting_tempo": 200,
    "starting_velocity": 64
})"""");
  QCOMPARE(chords_model.rowCount(), 0);

  import_musicxml(song_widget, test_dir.filePath("prelude.musicxml"));
  QCOMPARE(chords_model.rowCount(), MUSIC_XML_ROWS);

  import_musicxml(song_widget, test_dir.filePath("percussion.musicxml"));
  QCOMPARE(chords_model.rowCount(), PERCUSSION_ROWS);

  close_message_later(*this, "No chords");
  import_musicxml(song_widget, test_dir.filePath("empty.musicxml"));

  close_message_later(*this, "Notes without durations not supported");
  import_musicxml(song_widget, test_dir.filePath("MozartPianoSonata.musicxml"));

  close_message_later(*this, "Transposition not supported");
  import_musicxml(song_widget, test_dir.filePath("MozartTrio.musicxml"));

  close_message_later(*this, "Justly only supports partwise musicxml scores");
  import_musicxml(song_widget, test_dir.filePath("timewise.musicxml"));
};

auto main(int number_of_arguments, char *argument_strings[]) -> int {
  Q_ASSERT(number_of_arguments >= 2);
  QApplication app(number_of_arguments, argument_strings);
  Tester test_object;
  test_object.test_folder = QList<QString>(
      argument_strings, argument_strings + number_of_arguments)[1];
  if (number_of_arguments > 2) {
    std::copy(argument_strings + 2, argument_strings + number_of_arguments,
              argument_strings + 1);
  }
  return QTest::qExec(&test_object, number_of_arguments - 1, argument_strings);
}

#include "test.moc"