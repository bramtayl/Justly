#include <QtCore/QAbstractItemModel>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
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
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QUndoStack>
#include <QtTest/QTest>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>
#include <algorithm> // IWYU pragma: keep
#include <vector>

#include "column_numbers/ChordColumn.hpp"
#include "column_numbers/PitchedNoteColumn.hpp"
#include "column_numbers/UnpitchedNoteColumn.hpp"
#include "menus/EditMenu.hpp"
#include "menus/FileMenu.hpp"
#include "menus/InsertMenu.hpp"
#include "menus/PasteMenu.hpp"
#include "menus/PlayMenu.hpp"
#include "menus/SongMenuBar.hpp"
#include "menus/ViewMenu.hpp"
#include "other/Song.hpp"
#include "other/SongEditor.hpp"
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/UnpitchedNote.hpp"
#include "tables/ChordsTable.hpp"
#include "tables/PitchedNotesTable.hpp"
#include "tables/UnpitchedNotesTable.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"

static const auto BIG_VELOCITY = 126;
static const auto PERCUSSION_ROWS = 16;
static const auto MUSIC_XML_ROWS = 545;
static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;
static const auto NUMERATOR_SHIFT_TIMES = 6;
static const auto OCTAVE_SHIFT_TIMES = 9;
static const auto SELECT_AND_CLEAR =
    QItemSelectionModel::Select | QItemSelectionModel::Clear;
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

static void select_and_trigger(QAction &play_action,
                               QItemSelectionModel &selector,
                               const QModelIndex &index) {
  selector.select(index, SELECT_AND_CLEAR);
  Q_ASSERT(selector.selection().size() == 1);
  play_action.trigger();
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

static void test_previous_next_chord(ViewMenu &view_menu,
                                     SwitchColumn &switch_column,
                                     const int chord_number) {
  QCOMPARE(get_parent_chord_number(switch_column), chord_number);
  view_menu.previous_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_column), chord_number - 1);
  view_menu.next_chord_action.trigger();
  QCOMPARE(get_parent_chord_number(switch_column), chord_number);
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

static void test_new_value(QUndoStack &undo_stack, const QModelIndex &index,
                           const QVariant &old_value) {
  QCOMPARE_NE(old_value, index.data());
  undo_stack.undo();
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

  auto &undo_stack = song_widget.undo_stack;

  auto &edit_menu = song_menu_bar.edit_menu;
  auto &play_menu = song_menu_bar.play_menu;

  auto &paste_menu = edit_menu.paste_menu;
  auto &insert_menu = edit_menu.insert_menu;
  auto &remove_rows_action = edit_menu.remove_rows_action;
  auto &copy_action = edit_menu.copy_action;

  auto &paste_over_action = paste_menu.paste_over_action;

  auto &play_action = play_menu.play_action;

  for (const auto &row : column_header_rows) {
    QCOMPARE(model.headerData(row.column_number, Qt::Horizontal),
             row.column_name);
  }

  const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  const auto editable_flags = uneditable_flags | Qt::ItemIsEditable;

  for (const auto &row : flag_rows) {
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
    undo_stack.undo();
    QCOMPARE(full_index.data(), full_value);
  }

  const auto number_of_columns = model.columnCount();
  for (auto column_number = 0; column_number < number_of_columns;
       column_number = column_number + 1) {
    const auto delete_index = model.index(full_row_number, column_number);
    const auto &old_value = delete_index.data();

    select_and_trigger(edit_menu.delete_cells_action, selector, delete_index);
    test_new_value(undo_stack, delete_index, old_value);
  }

  for (const auto &row :
       get_index_pairs(model, empty_row_number, full_row_number)) {
    const auto &empty_index = row.first_index;
    const auto &full_index = row.second_index;

    const auto empty_value = empty_index.data();
    const auto full_value = full_index.data();

    QCOMPARE_NE(empty_value, full_value);

    selector.select(empty_index, SELECT_AND_CLEAR);
    copy_action.trigger();

    selector.select(full_index, SELECT_AND_CLEAR);
    paste_over_action.trigger();

    QCOMPARE(full_index.data(), empty_value);
    undo_stack.undo();
    QCOMPARE(full_index.data(), full_value);

    selector.select(full_index, SELECT_AND_CLEAR);
    edit_menu.cut_action.trigger();

    QCOMPARE(full_index.data(), empty_value);

    selector.select(empty_index, SELECT_AND_CLEAR);
    paste_over_action.trigger();

    QCOMPARE(empty_index.data(), full_value);
    undo_stack.undo();
    QCOMPARE(empty_index.data(), empty_value);
    undo_stack.undo();
    QCOMPARE(full_index.data(), full_value);
  }

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  copy_action.trigger();

  const auto number_of_rows = model.rowCount();
  paste_menu.paste_into_action.trigger();
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), number_of_rows);

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  paste_menu.paste_after_action.trigger();
  QCOMPARE(model.rowCount(), number_of_rows + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), number_of_rows);

  const auto chord_index = model.index(0, chord_interval_column);
  const auto old_child_row_count = model.rowCount(chord_index);
  selector.select(chord_index, SELECT_AND_CLEAR);
  insert_menu.insert_into_action.trigger();

  QCOMPARE(model.rowCount(chord_index), old_child_row_count + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(chord_index), old_child_row_count);

  const auto index = model.index(0, 0);
  const auto old_row_count = model.rowCount();

  selector.select(index, SELECT_AND_CLEAR);
  insert_menu.insert_after_action.trigger();

  QCOMPARE(model.rowCount(), old_row_count + 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), old_row_count);

  selector.select(model.index(0, 0), SELECT_AND_CLEAR);
  remove_rows_action.trigger();

  QCOMPARE(model.rowCount(), old_row_count - 1);
  undo_stack.undo();
  QCOMPARE(model.rowCount(), old_row_count);

  for (const auto &row : play_rows) {
    selector.select(QItemSelection(row.first_index, row.second_index),
                    SELECT_AND_CLEAR);
    play_action.trigger();
    // first cut off early
    play_action.trigger();
    // now play for a while
    QThread::msleep(WAIT_TIME);
    play_menu.stop_playing_action.trigger();
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
    paste_over_action.trigger();
  }
};

static void test_buttons(SongWidget &song_widget, QAbstractItemView &table,
                         int interval_column) {
  const auto &model = get_reference(table.model());
  auto &selector = get_reference(table.selectionModel());
  const auto first_index = model.index(0, interval_column);
  const auto original_data = first_index.data();
  selector.select(first_index, SELECT_AND_CLEAR);

  auto &undo_stack = song_widget.undo_stack;
  auto &controls_column = song_widget.controls_column;
  auto &third_row = controls_column.third_row;
  auto &fifth_row = controls_column.fifth_row;
  auto &seventh_row = controls_column.seventh_row;
  auto &octave_row = controls_column.octave_row;

  third_row.minus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  third_row.plus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  fifth_row.minus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  fifth_row.plus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  seventh_row.minus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  seventh_row.plus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  octave_row.minus_button.click();
  test_new_value(undo_stack, first_index, original_data);

  octave_row.plus_button.click();
  test_new_value(undo_stack, first_index, original_data);
}

static void octave_up_times(QPushButton &plus_button, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    plus_button.click();
  }
}

static void undo_times(QUndoStack &undo_stack, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    undo_stack.undo();
  }
}

void Tester::run_tests() {
  set_up();

  SongEditor song_editor;

  auto &song_widget = song_editor.song_widget;
  auto &song_menu_bar = song_editor.song_menu_bar;

  auto &switch_column = song_widget.switch_column;

  auto &chords_table = switch_column.chords_table;
  auto &pitched_notes_table = switch_column.pitched_notes_table;
  auto &unpitched_notes_table = switch_column.unpitched_notes_table;

  auto &chords_model = *(chords_table.model());
  auto &pitched_notes_model = *(pitched_notes_table.model());
  auto &unpitched_notes_model = *(unpitched_notes_table.model());

  auto &chords_selector = *(chords_table.selectionModel());
  auto &pitched_notes_selector = *(pitched_notes_table.selectionModel());
  auto &unpitched_notes_selector = *(unpitched_notes_table.selectionModel());

  auto &undo_stack = song_widget.undo_stack;

  auto &song = song_widget.song;
  auto &controls_column = song_widget.controls_column;

  auto &spin_boxes = controls_column.spin_boxes;
  auto &fifth_row = controls_column.fifth_row;
  auto &octave_row = controls_column.octave_row;

  auto &gain_editor = spin_boxes.gain_editor;
  auto &starting_key_editor = spin_boxes.starting_key_editor;
  auto &starting_velocity_editor = spin_boxes.starting_velocity_editor;
  auto &starting_tempo_editor = spin_boxes.starting_tempo_editor;

  auto &fifth_minus_button = fifth_row.minus_button;
  auto &fifth_plus_button = fifth_row.plus_button;

  auto &octave_minus_button = octave_row.minus_button;
  auto &octave_plus_button = octave_row.plus_button;

  auto &view_menu = song_menu_bar.view_menu;
  auto &play_menu = song_menu_bar.play_menu;

  auto &back_to_chords_action = view_menu.back_to_chords_action;

  auto &delete_cells_action = song_menu_bar.edit_menu.delete_cells_action;

  auto &play_action = play_menu.play_action;

  QDir test_dir(test_folder);

  const auto test_song_file = test_dir.filePath("test_song.xml");

  open_file(song_widget, test_song_file);

  // test buttons
  test_buttons(song_widget, chords_table, chord_interval_column);

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  test_buttons(song_widget, pitched_notes_table, pitched_note_interval_column);
  undo_stack.undo(); // back to chords

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
    undo_stack.undo();
  }

  for (const auto &row :
       std::vector({CountRow({0, 0}), CountRow({1, 4}), CountRow({2, 0}),
                    CountRow({3, 0}), CountRow({4, 0}), CountRow({5, 0}),
                    CountRow({6, 0}), CountRow({7, 0})})) {
    double_click_column(chords_table, row.chord_number,
                        chord_unpitched_notes_column);
    QCOMPARE(unpitched_notes_model.rowCount(), row.number);
    undo_stack.undo();
  }

  double_click_column(chords_table, 0, chord_unpitched_notes_column);
  back_to_chords_action.trigger();
  undo_stack.undo();
  undo_stack.undo();

  test_number_of_columns(chords_model, number_of_chord_columns);
  test_number_of_columns(pitched_notes_model, number_of_pitched_note_columns);
  test_number_of_columns(unpitched_notes_model,
                         number_of_unpitched_note_columns);

  const auto old_gain = get_gain(song_widget);
  QCOMPARE_NE(old_gain, NEW_GAIN_1);
  QCOMPARE_NE(old_gain, NEW_GAIN_2);

  gain_editor.setValue(NEW_GAIN_1);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
  gain_editor.setValue(NEW_GAIN_2);
  QCOMPARE(get_gain(song_widget), NEW_GAIN_2);

  undo_stack.undo();
  QCOMPARE(get_gain(song_widget), old_gain);

  const auto old_key = song.starting_key;
  QCOMPARE_NE(old_key, STARTING_KEY_1);
  QCOMPARE_NE(old_key, STARTING_KEY_2);

  // test combining
  starting_key_editor.setValue(STARTING_KEY_1);
  QCOMPARE(song.starting_key, STARTING_KEY_1);
  starting_key_editor.setValue(STARTING_KEY_2);
  QCOMPARE(song.starting_key, STARTING_KEY_2);
  undo_stack.undo();
  QCOMPARE(song.starting_key, old_key);

  const auto old_velocity = song.starting_velocity;
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_1);
  QCOMPARE_NE(old_velocity, STARTING_VELOCITY_2);

  // test combining
  starting_velocity_editor.setValue(STARTING_VELOCITY_1);
  QCOMPARE(song.starting_velocity, STARTING_VELOCITY_1);
  starting_velocity_editor.setValue(STARTING_VELOCITY_2);
  QCOMPARE(song.starting_velocity, STARTING_VELOCITY_2);
  undo_stack.undo();
  QCOMPARE(song.starting_velocity, old_velocity);

  const auto old_tempo = song.starting_tempo;

  // test combining
  starting_tempo_editor.setValue(STARTING_TEMPO_1);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_1);
  starting_tempo_editor.setValue(STARTING_TEMPO_2);
  QCOMPARE(song.starting_tempo, STARTING_TEMPO_2);
  undo_stack.undo();
  QCOMPARE(song.starting_tempo, old_tempo);

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
    starting_key_editor.setValue(row.frequency);
    QCOMPARE(
        chords_model.index(0, chord_interval_column).data(Qt::StatusTipRole),
        row.text);
    undo_stack.undo();
  }

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  starting_key_editor.setValue(A_FREQUENCY);
  QCOMPARE(pitched_notes_model.index(0, pitched_note_interval_column)
               .data(Qt::StatusTipRole),
           "660 Hz ≈ E5 + 2 cents; Velocity 30; 300 bpm; Start at 600 ms");
  undo_stack.undo();
  undo_stack.undo();

  double_click_column(chords_table, 1, chord_unpitched_notes_column);
  QCOMPARE(unpitched_notes_model.index(0, 0).data(Qt::StatusTipRole), "");
  undo_stack.undo();

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
      std::vector({BadPasteRow({"", "not a mime",
                                "Cannot paste not a mime as "
                                "chords cells"}),
                   BadPasteRow({"", PitchedNote::get_cells_mime(),
                                "Cannot paste pitched notes cells as "
                                "chords cells"}),
                   BadPasteRow({"[", Chord::get_cells_mime(), "Invalid XML"}),
                   BadPasteRow({"<song/>", Chord::get_cells_mime(),
                                "Invalid clipboard"})}),
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
           BadPasteRow({"", Chord::get_cells_mime(),
                        "Cannot paste chords cells as "
                        "pitched notes cells"}),
           BadPasteRow({"<", PitchedNote::get_cells_mime(), "Invalid XML"}),
           BadPasteRow({"<song/>", PitchedNote::get_cells_mime(),
                        "Invalid clipboard"})}),
      0, 1);
  undo_stack.undo();

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
           BadPasteRow({"", Chord::get_cells_mime(),
                        "Cannot paste chords cells as "
                        "unpitched notes cells"}),
           BadPasteRow({"<", UnpitchedNote::get_cells_mime(), "Invalid XML"}),
           BadPasteRow({"<song/>", UnpitchedNote::get_cells_mime(),
                        "Invalid clipboard"})}),
      0, 1);
  undo_stack.undo(); // back to chords

  double_click_column(chords_table, 1, chord_pitched_notes_column);

  starting_velocity_editor.setValue(BIG_VELOCITY);

  close_message_later(*this,
                      "Velocity 378 exceeds 127 for chord 2, pitched note 1");

  select_and_trigger(
      play_action, pitched_notes_selector,
      pitched_notes_model.index(0, pitched_note_interval_column));

  QThread::msleep(WAIT_TIME);
  play_menu.stop_playing_action.trigger();
  undo_stack.undo(); // undo set starting velocity

  pitched_notes_selector.select(
      pitched_notes_model.index(0, pitched_note_interval_column),
      SELECT_AND_CLEAR);

  // test frequency out of bounds
  close_message_later(*this,
                      "Frequency 337920 for chord 2, pitched note 1 greater "
                      "than or equal to maximum frequency 12911.4");
  octave_up_times(octave_plus_button, OCTAVE_SHIFT_TIMES);
  select_and_trigger(
      play_action, pitched_notes_selector,
      pitched_notes_model.index(0, pitched_note_interval_column));
  undo_times(undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave

  close_message_later(*this, "Frequency 1.28906 for chord 2, pitched note 1 "
                             "less than minimum frequency 7.94305");
  for (auto counter = 0; counter < OCTAVE_SHIFT_TIMES; counter++) {
    octave_minus_button.click();
  }
  select_and_trigger(
      play_action, pitched_notes_selector,
      pitched_notes_model.index(0, pitched_note_interval_column));
  undo_times(undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave

  // test interval components out of bounds
  for (auto counter = 0; counter < NUMERATOR_SHIFT_TIMES; counter++) {
    fifth_plus_button.click();
    octave_plus_button.click();
  }
  close_message_later(*this, "Numerator 2187 greater than maximum 999");
  fifth_plus_button.click();
  undo_times(undo_stack, NUMERATOR_SHIFT_TIMES * 2); // undo shift numerator

  for (auto counter = 0; counter < NUMERATOR_SHIFT_TIMES; counter++) {
    fifth_minus_button.click();
    octave_minus_button.click();
  }
  close_message_later(*this, "Denominator 2187 greater than maximum 999");
  fifth_minus_button.click();
  undo_times(undo_stack, NUMERATOR_SHIFT_TIMES * 2); // undo shift denominator

  close_message_later(*this, "Octave 10 (absolutely) greater than maximum 9");
  octave_up_times(octave_plus_button, OCTAVE_SHIFT_TIMES + 1);
  undo_times(undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave

  undo_stack.undo(); // undo back to chords

  select_and_trigger(delete_cells_action, chords_selector,
                     chords_model.index(1, chord_instrument_column));

  double_click_column(chords_table, 1, chord_pitched_notes_column);

  const auto instrument_delete_index =
      pitched_notes_model.index(1, pitched_note_instrument_column);

  select_and_trigger(delete_cells_action, pitched_notes_selector,
                     instrument_delete_index);
  QCOMPARE(instrument_delete_index.data().toString(), "");
  close_message_later(*this, "No instrument for chord 2, pitched note 2");

  select_and_trigger(play_action, pitched_notes_selector,
                     instrument_delete_index);

  // undo delete pitched_note instrument
  undo_stack.undo();
  // undo edit pitched_notes
  undo_stack.undo();
  // undo delete chord instrument
  undo_stack.undo();

  select_and_trigger(delete_cells_action, chords_selector,
                     chords_model.index(1, chord_percussion_instrument_column));

  double_click_column(chords_table, 1, chord_unpitched_notes_column);

  const auto percussion_instrument_delete_index = unpitched_notes_model.index(
      1, unpitched_note_percussion_instrument_column);

  select_and_trigger(delete_cells_action, unpitched_notes_selector,
                     percussion_instrument_delete_index);

  close_message_later(*this, "No percussion set for chord 2, unpitched note 2");

  select_and_trigger(play_action, unpitched_notes_selector,
                     percussion_instrument_delete_index);
  // undo edit delete unpitched_note set
  undo_stack.undo();

  select_and_trigger(delete_cells_action, unpitched_notes_selector,
                     percussion_instrument_delete_index);

  close_message_later(*this, "No percussion set for chord 2, "
                             "unpitched note 2");

  select_and_trigger(play_action, unpitched_notes_selector,
                     percussion_instrument_delete_index);
  // undo delete unpitched_note percussion instrument
  undo_stack.undo();
  // undo edit unpitched_notes
  undo_stack.undo();
  // undo delete chord percussion instrument
  undo_stack.undo();
  // undo delete chord percussion set
  undo_stack.undo();

  double_click_column(chords_table, 1, chord_unpitched_notes_column);
  test_previous_next_chord(view_menu, switch_column, 1);
  back_to_chords_action.trigger();

  double_click_column(chords_table, 1, chord_pitched_notes_column);
  test_previous_next_chord(view_menu, switch_column, 1);
  back_to_chords_action.trigger();

  QTemporaryFile temp_json_file;
  QVERIFY(temp_json_file.open());
  temp_json_file.close();
  const auto file_name = temp_json_file.fileName();
  save_as_file(song_widget, file_name);

  QCOMPARE(song_widget.current_file, file_name);

  QVERIFY(temp_json_file.open());
  QString written(temp_json_file.readAll());
  temp_json_file.close();

  written.replace("\r\n", "\n");

  QFile test_song_qfile(test_song_file);

  QVERIFY(test_song_qfile.open(QIODevice::ReadOnly));
  QString original(test_song_qfile.readAll());
  test_song_qfile.close();

  original.replace("\r\n", "\n");

  QCOMPARE(original, written);
  song_menu_bar.file_menu.save_action.trigger();

  QTemporaryFile temp_export_file;
  QVERIFY(temp_export_file.open());
  temp_export_file.close();
  export_to_file(song_widget, temp_export_file.fileName());

  for (const auto &row :
       std::vector({TwoStringsRow({"<", "Invalid XML file"}),
                    TwoStringsRow({"<song/>", "Invalid song file"})})) {
    close_message_later(*this, row.second_string);
    open_text(song_widget, row.first_string);
  }

  open_text(song_widget, R""""(<song>
    <gain>5.0</gain>
    <starting_key>220</starting_key>
    <starting_tempo>200</starting_tempo>
    <starting_velocity>64</starting_velocity>
</song>
)"""");
  QCOMPARE(chords_model.rowCount(), 0);

  import_musicxml(song_widget, test_dir.filePath("prelude.musicxml"));
  QCOMPARE(chords_model.rowCount(), MUSIC_XML_ROWS);

  import_musicxml(song_widget, test_dir.filePath("percussion.musicxml"));
  QCOMPARE(chords_model.rowCount(), PERCUSSION_ROWS);

  close_message_later(*this, "Invalid musicxml file");
  import_musicxml(song_widget, test_dir.filePath("not_musicxml.xml"));

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
  QApplication app(number_of_arguments, argument_strings);
  Tester test_object;
  QDir folder(QCoreApplication::applicationDirPath());
  folder.cdUp();
  folder.cd("share");
  test_object.test_folder = folder.absolutePath();
  return QTest::qExec(&test_object, number_of_arguments, argument_strings);
}

#include "test.moc"