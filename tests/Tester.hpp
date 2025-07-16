#pragma once

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
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/Qt>
#include <QtCore/QtGlobal>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QUndoStack>
#include <QtTest/QTest>
#include <QtTest/QTestData>
#include <QtWidgets/QAbstractItemDelegate>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStyleOption>
#include <QtWidgets/QWidget>

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
#include "other/helpers.hpp"
#include "rows/Chord.hpp"
#include "rows/PitchedNote.hpp"
#include "rows/RowType.hpp"
#include "rows/UnpitchedNote.hpp"
#include "widgets/ControlsColumn.hpp"
#include "widgets/IntervalRow.hpp"
#include "widgets/SongEditor.hpp"
#include "widgets/SongWidget.hpp"
#include "widgets/SpinBoxes.hpp"
#include "widgets/SwitchColumn.hpp"
#include "widgets/SwitchTable.hpp"

static const auto BIG_VELOCITY = 126;
static const auto PERCUSSION_ROWS = 16;
static const auto MUSIC_XML_ROWS = 545;
static const auto NEW_GAIN_1 = 2;
static const auto NEW_GAIN_2 = 3;
static const auto RATIO_SHIFT_TIMES = 6;
static const auto OCTAVE_SHIFT_TIMES = 9;
static const auto SELECT_AND_CLEAR =
    QItemSelectionModel::Select | QItemSelectionModel::Clear;
static const auto STARTING_KEY_1 = 401.0;
static const auto STARTING_KEY_2 = 402.0;
static const auto STARTING_TEMPO_1 = 150.0;
static const auto STARTING_TEMPO_2 = 125.0;
static const auto STARTING_VELOCITY_1 = 70.0;
static const auto STARTING_VELOCITY_2 = 80.0;
static const auto WAIT_TIME = 500;

static const auto A_MINUS_FREQUENCY = 217;
static const auto A_PLUS_FREQUENCY = 223;
static const auto A_FREQUENCY = 220;
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

static const auto SIX = 6;
static const auto EIGHT = 8;

static auto get_model(QAbstractItemView &table) -> auto & {
  return get_reference(table.model());
}

static void double_click_column(QAbstractItemView &table, const int row_number,
                                const int column_number) {
  table.doubleClicked(get_model(table).index(row_number, column_number));
}

static void select_cell(QAbstractItemView &table, const int row,
                        const int column) {
  get_selection_model(table).select(get_model(table).index(row, column),
                                    SELECT_AND_CLEAR);
}

static void switch_to(QAbstractItemView &switch_table, const RowType row_type,
                      const int chord_number) {
  switch (row_type) {
  case chord_type:
    QVERIFY(chord_number == -1);
    break;
  case pitched_note_type:
    double_click_column(switch_table, chord_number,
                        static_cast<int>(chord_pitched_notes_column));
    break;
  case unpitched_note_type:
    double_click_column(switch_table, chord_number,
                        static_cast<int>(chord_unpitched_notes_column));
  }
}

static void maybe_switch_back_to_chords(QUndoStack &undo_stack,
                                        const RowType row_type) {
  if (row_type != chord_type) {
    undo_stack.undo();
  }
}

static void open_text(SongWidget &song_widget, const QString &song_text) {
  QTemporaryFile temp_file;
  QVERIFY(temp_file.open());
  temp_file.write(song_text.toStdString().c_str());
  temp_file.close();
  open_file(song_widget, temp_file.fileName());
}

static void close_message_later(QWidget &parent, bool &waiting_for_message,
                                const QString &expected_text) {
  const auto waiting_before = waiting_for_message;
  waiting_for_message = true;
  auto &timer = // NOLINT(cppcoreguidelines-owning-memory)
      *(new QTimer(&parent));
  timer.setSingleShot(true);
  QObject::connect(
      &timer, &QTimer::timeout, &parent,
      [expected_text, &waiting_for_message]() {
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
  timer.start(WAIT_TIME);
  QVERIFY(!waiting_before);
};

static void press_times(QPushButton &plus_button, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    plus_button.click();
  }
}

static void undo_times(QUndoStack &undo_stack, const int count) {
  for (auto counter = 0; counter < count; counter++) {
    undo_stack.undo();
  }
}

static void add_table_columns() {
  QTest::addColumn<RowType>("row_type");
  QTest::addColumn<int>("chord_number");
}

static void add_tables() {
  add_table_columns();

  QTest::newRow("chord") << chord_type << -1;
  QTest::newRow("pitched note") << pitched_note_type << 1;
  QTest::newRow("unpitched note") << unpitched_note_type << 1;
}

static void add_cells() {
  add_table_columns();
  QTest::addColumn<int>("row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord pitched notes")
      << chord_type << -1 << 1 << static_cast<int>(chord_pitched_notes_column);
  QTest::newRow("chord unpitched notes")
      << chord_type << -1 << 1
      << static_cast<int>(chord_unpitched_notes_column);
  QTest::newRow("chord instrument")
      << chord_type << -1 << 1 << static_cast<int>(chord_instrument_column);
  QTest::newRow("chord percussion instrument")
      << chord_type << -1 << 1
      << static_cast<int>(chord_percussion_instrument_column);
  QTest::newRow("chord interval")
      << chord_type << -1 << 1 << static_cast<int>(chord_interval_column);
  QTest::newRow("chord beats")
      << chord_type << -1 << 1 << static_cast<int>(chord_beats_column);
  QTest::newRow("chord velocity ratio")
      << chord_type << -1 << 1 << static_cast<int>(chord_velocity_ratio_column);
  QTest::newRow("chord tempo ratio")
      << chord_type << -1 << 1 << static_cast<int>(chord_tempo_ratio_column);
  QTest::newRow("chord words")
      << chord_type << -1 << 1 << static_cast<int>(chord_words_column);
  QTest::newRow("pitched note instrument")
      << pitched_note_type << 1 << 1
      << static_cast<int>(pitched_note_instrument_column);
  QTest::newRow("pitched note interval")
      << pitched_note_type << 1 << 1
      << static_cast<int>(pitched_note_interval_column);
  QTest::newRow("pitched note beats")
      << pitched_note_type << 1 << 1
      << static_cast<int>(pitched_note_beats_column);
  QTest::newRow("pitched note velocity ratio")
      << pitched_note_type << 1 << 1
      << static_cast<int>(pitched_note_velocity_ratio_column);
  QTest::newRow("pitched note words")
      << pitched_note_type << 1 << 1
      << static_cast<int>(pitched_note_words_column);
  QTest::newRow("unpitched note percussion instrument")
      << unpitched_note_type << 1 << 1
      << static_cast<int>(unpitched_note_percussion_instrument_column);
  QTest::newRow("unpitched note beats")
      << unpitched_note_type << 1 << 1
      << static_cast<int>(unpitched_note_beats_column);
  QTest::newRow("unpitched note velocity ratio")
      << unpitched_note_type << 1 << 1
      << static_cast<int>(unpitched_note_velocity_ratio_column);
  QTest::newRow("unpitched note words")
      << unpitched_note_type << 1 << 1
      << static_cast<int>(unpitched_note_words_column);
}

static void add_editable_cell_pairs() {
  add_table_columns();
  QTest::addColumn<int>("first_row_number");
  QTest::addColumn<int>("second_row_number");
  QTest::addColumn<int>("column_number");

  QTest::newRow("chord instrument")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_instrument_column);
  QTest::newRow("chord percussion instrument")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_percussion_instrument_column);
  QTest::newRow("chord interval")
      << chord_type << -1 << 0 << 1 << static_cast<int>(chord_interval_column);
  QTest::newRow("chord beats")
      << chord_type << -1 << 0 << 1 << static_cast<int>(chord_beats_column);
  QTest::newRow("chord velocity ratio")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_velocity_ratio_column);
  QTest::newRow("chord tempo ratio")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_tempo_ratio_column);
  QTest::newRow("chord words")
      << chord_type << -1 << 0 << 1 << static_cast<int>(chord_words_column);
  QTest::newRow("pitched note instrument")
      << pitched_note_type << 1 << 0 << 1
      << static_cast<int>(pitched_note_instrument_column);
  QTest::newRow("pitched note interval")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(pitched_note_interval_column);
  QTest::newRow("pitched note beats")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(pitched_note_beats_column);
  QTest::newRow("pitched note velocity ratio")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(pitched_note_velocity_ratio_column);
  QTest::newRow("pitched note words")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(unpitched_note_words_column);
  QTest::newRow("unpitched note percussion instrument")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(unpitched_note_percussion_instrument_column);
  QTest::newRow("unpitched note beats")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(unpitched_note_beats_column);
  QTest::newRow("unpitched note velocity ratio")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(unpitched_note_velocity_ratio_column);
  QTest::newRow("unpitched note words")
      << unpitched_note_type << 1 << 0 << 1
      << static_cast<int>(unpitched_note_words_column);
}

static void add_cell_pairs() {
  add_editable_cell_pairs();
  QTest::newRow("chord pitched notes")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_pitched_notes_column);
  QTest::newRow("chord unpitched notes")
      << chord_type << -1 << 0 << 1
      << static_cast<int>(chord_unpitched_notes_column);
}

static auto get_file_text(const QString &filename) {
  QFile file(filename);
  Q_ASSERT(file.open(QIODevice::ReadOnly));
  QString file_text(file.readAll());
  file.close();
  // normalize line endings
  file_text.replace("\r\n", "\n");
  return file_text;
}

struct Tester : public QObject {
  Q_OBJECT
public:
  SongEditor song_editor;
  QDir test_dir = []() {
    QDir test_dir(QCoreApplication::applicationDirPath());
    test_dir.cdUp();
    test_dir.cd("share");
    return test_dir;
  }();

  Tester() {
    set_up();
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  }
  bool waiting_for_message = false;

private slots:
  static void test_column_count_data() {
    add_table_columns();
    QTest::addColumn<int>("number_of_columns");

    QTest::newRow("chord") << chord_type << -1
                           << static_cast<int>(number_of_chord_columns);
    QTest::newRow("pitched note")
        << pitched_note_type << 0
        << static_cast<int>(number_of_pitched_note_columns);
    QTest::newRow("unpitched note")
        << unpitched_note_type << 0
        << static_cast<int>(number_of_unpitched_note_columns);
  };

  void test_column_count() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, number_of_columns);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_model(switch_table).columnCount(), number_of_columns);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_column_header_data() {
    add_table_columns();
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QString>("column_name");

    QTest::newRow("chord instrument")
        << chord_type << -1 << static_cast<int>(chord_instrument_column)
        << "Instrument";
    QTest::newRow("chord percussion instrument")
        << chord_type << -1
        << static_cast<int>(chord_percussion_instrument_column)
        << "Percussion instrument";
    QTest::newRow("chord interval")
        << chord_type << -1 << static_cast<int>(chord_interval_column)
        << "Interval";
    QTest::newRow("chord beats")
        << chord_type << -1 << static_cast<int>(chord_beats_column) << "Beats";
    QTest::newRow("chord velocity ratio")
        << chord_type << -1 << static_cast<int>(chord_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("chord tempo ratio")
        << chord_type << -1 << static_cast<int>(chord_tempo_ratio_column)
        << "Tempo ratio";
    QTest::newRow("chord words")
        << chord_type << -1 << static_cast<int>(chord_words_column) << "Words";
    QTest::newRow("chord pitched notes")
        << chord_type << -1 << static_cast<int>(chord_pitched_notes_column)
        << "Pitched notes";
    QTest::newRow("chord unpitched notes")
        << chord_type << -1 << static_cast<int>(chord_unpitched_notes_column)
        << "Unpitched notes";
    QTest::newRow("pitched note instrument")
        << pitched_note_type << 1
        << static_cast<int>(pitched_note_instrument_column) << "Instrument";
    QTest::newRow("pitched note interval")
        << pitched_note_type << 1
        << static_cast<int>(pitched_note_interval_column) << "Interval";
    QTest::newRow("pitched note beats")
        << pitched_note_type << 1 << static_cast<int>(pitched_note_beats_column)
        << "Beats";
    QTest::newRow("pitched note velocity ratio")
        << pitched_note_type << 1
        << static_cast<int>(pitched_note_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("pitched note words")
        << pitched_note_type << 1 << static_cast<int>(pitched_note_words_column)
        << "Words";
    QTest::newRow("unpitched note percussion instrument")
        << unpitched_note_type << 1
        << static_cast<int>(unpitched_note_percussion_instrument_column)
        << "Percussion instrument";
    QTest::newRow("unpitched note beats")
        << unpitched_note_type << 1
        << static_cast<int>(unpitched_note_beats_column) << "Beats";
    QTest::newRow("unpitched note velocity ratio")
        << unpitched_note_type << 1
        << static_cast<int>(unpitched_note_velocity_ratio_column)
        << "Velocity ratio";
    QTest::newRow("unpitched note words")
        << unpitched_note_type << 1
        << static_cast<int>(unpitched_note_words_column) << "Words";
  };

  void test_column_header() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, column_number);
    QFETCH(QString, column_name);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_model(switch_table).headerData(column_number, Qt::Horizontal),
             column_name);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_copy_data() { add_cell_pairs(); };

  void test_copy() {

    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, first_row_number);
    QFETCH(int, second_row_number);
    QFETCH(int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);
    auto &selector = get_selection_model(switch_table);

    const auto &first_index = model.index(first_row_number, column_number);
    const auto &second_index = model.index(second_row_number, column_number);

    const auto first_value = first_index.data();
    const auto second_value = second_index.data();

    QCOMPARE_NE(first_value, second_value);

    selector.select(first_index, SELECT_AND_CLEAR);
    edit_menu.copy_action.trigger();

    selector.select(second_index, SELECT_AND_CLEAR);
    edit_menu.paste_menu.paste_over_action.trigger();

    QCOMPARE(second_index.data(), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_cut_data() { add_cell_pairs(); };

  void test_cut() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, first_row_number);
    QFETCH(int, second_row_number);
    QFETCH(int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);
    auto &selector = get_selection_model(switch_table);

    const auto &first_index = model.index(first_row_number, column_number);
    const auto &second_index = model.index(second_row_number, column_number);

    const auto first_value = first_index.data();
    const auto second_value = second_index.data();

    QCOMPARE_NE(first_value, second_value);

    selector.select(second_index, SELECT_AND_CLEAR);
    edit_menu.cut_action.trigger();

    QCOMPARE(second_index.data(), first_value);

    selector.select(first_index, SELECT_AND_CLEAR);
    edit_menu.paste_menu.paste_over_action.trigger();

    QCOMPARE(first_index.data(), second_value);
    undo_stack.undo();
    QCOMPARE(first_index.data(), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_delete_data() { add_cells(); };

  void test_delete() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, row_number);
    QFETCH(int, column_number);

    switch_to(switch_table, row_type, chord_number);

    const auto delete_index =
        get_model(switch_table).index(row_number, column_number);
    const auto &old_value = delete_index.data();

    get_selection_model(switch_table).select(delete_index, SELECT_AND_CLEAR);
    song_editor.song_menu_bar.edit_menu.delete_cells_action.trigger();

    QCOMPARE_NE(old_value, delete_index.data());
    undo_stack.undo();
    QCOMPARE(old_value, delete_index.data());

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_export() {
    auto &song_widget = song_editor.song_widget;

    QTemporaryFile temp_export_file;
    QVERIFY(temp_export_file.open());
    temp_export_file.close();
    export_to_file(song_widget, temp_export_file.fileName());
  };

  static void test_flag_data() {
    add_table_columns();
    QTest::addColumn<int>("column_number");
    QTest::addColumn<bool>("is_editable");

    QTest::newRow("chord interval")
        << chord_type << -1 << static_cast<int>(chord_interval_column) << true;
    QTest::newRow("chord pitched notes")
        << chord_type << -1 << static_cast<int>(chord_pitched_notes_column)
        << false;
    QTest::newRow("chord unpitched notes")
        << chord_type << -1 << static_cast<int>(chord_unpitched_notes_column)
        << false;
    QTest::newRow("pitched note") << pitched_note_type << 1 << 0 << true;
    QTest::newRow("unpitched note") << unpitched_note_type << 1 << 0 << true;
  };

  void test_flag() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, column_number);
    QFETCH(bool, is_editable);

    const auto uneditable_flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_model(switch_table).index(0, column_number).flags(),
             is_editable ? uneditable_flags | Qt::ItemIsEditable
                         : uneditable_flags);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  void test_frequency_bound_data() {
    QTest::addColumn<QPushButton *>("button_pointer");
    QTest::addColumn<QString>("error_message");

    auto &octave_row = song_editor.song_widget.controls_column.octave_row;

    QTest::newRow("too high")
        << &octave_row.plus_button
        << "Frequency 337920 for chord 2, pitched note 1 greater than or equal "
           "to maximum frequency 12911.4";
    QTest::newRow("too_low") << &octave_row.minus_button
                             << "Frequency 1.28906 for chord 2, pitched note 1 "
                                "less than minimum frequency 7.94305";
  }

  void test_frequency_bound() {
    QFETCH(QPushButton *, button_pointer);
    QFETCH(QString, error_message);
    auto &button = get_reference(button_pointer);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, pitched_note_type, 1);
    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message, error_message);
    press_times(button, OCTAVE_SHIFT_TIMES);
    song_editor.song_menu_bar.play_menu.play_action.trigger();
    undo_times(song_widget.undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave
    maybe_switch_back_to_chords(undo_stack, pitched_note_type);
  };

  static void test_frequency_in_status_data() {
    QTest::addColumn<int>("frequency");
    QTest::addColumn<QString>("text");

    QTest::newRow("just below A")
        << A_MINUS_FREQUENCY
        << "217 Hz ≈ A3 − 24 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
    QTest::newRow("just above A")
        << A_PLUS_FREQUENCY
        << "223 Hz ≈ A3 + 23 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
    QTest::newRow("A")
        << A_FREQUENCY
        << "220 Hz ≈ A3; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("B♭")
        << B_FLAT_FREQUENCY
        << "233 Hz ≈ B♭3 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
    QTest::newRow("B")
        << B_FREQUENCY
        << "247 Hz ≈ B3; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("C") << C_FREQUENCY
                       << "262 Hz ≈ C4 + 2 cents; Velocity 10; 100 "
                          "bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("C♯")
        << C_SHARP_FREQUENCY
        << "277 Hz ≈ C♯4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
    QTest::newRow("D") << D_FREQUENCY
                       << "294 Hz ≈ D4 + 2 cents; Velocity 10; 100 "
                          "bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("E♭")
        << E_FLAT_FREQUENCY
        << "311 Hz ≈ E♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
    QTest::newRow("E") << E_FREQUENCY
                       << "330 Hz ≈ E4 + 2 cents; Velocity 10; 100 "
                          "bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("F") << F_FREQUENCY
                       << "349 Hz ≈ F4 − 1 cents; Velocity 10; 100 "
                          "bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("F♯")
        << F_SHARP_FREQUENCY
        << "370 Hz ≈ F♯4; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("G")
        << G_FREQUENCY
        << "392 Hz ≈ G4; Velocity 10; 100 bpm; Start at 0 ms; Duration 600 ms";
    QTest::newRow("A♭")
        << A_FLAT_FREQUENCY
        << "415 Hz ≈ A♭4 − 1 cents; Velocity 10; 100 bpm; Start at 0 ms; "
           "Duration 600 ms";
  };

  void test_frequency_in_status() {
    QFETCH(int, frequency);
    QFETCH(QString, text);

    auto &song_widget = song_editor.song_widget;

    song_widget.controls_column.spin_boxes.starting_key_editor.setValue(
        frequency);
    QCOMPARE(get_model(song_widget.switch_column.switch_table)
                 .index(0, 0)
                 .data(Qt::StatusTipRole),
             text);
    song_widget.undo_stack.undo();
  };

  void test_gain() {
    auto &song_widget = song_editor.song_widget;
    auto &gain_editor = song_widget.controls_column.spin_boxes.gain_editor;

    const auto old_gain = get_gain(song_widget);
    QCOMPARE_NE(old_gain, NEW_GAIN_1);
    QCOMPARE_NE(old_gain, NEW_GAIN_2);

    gain_editor.setValue(NEW_GAIN_1);
    QCOMPARE(get_gain(song_widget), NEW_GAIN_1);
    gain_editor.setValue(NEW_GAIN_2);
    QCOMPARE(get_gain(song_widget), NEW_GAIN_2);

    song_widget.undo_stack.undo();
    QCOMPARE(get_gain(song_widget), old_gain);
  };

  static void test_insert_after_data() { add_tables(); };

  void test_insert_after() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();

    select_cell(switch_table, 0, 0);
    song_editor.song_menu_bar.edit_menu.insert_menu.insert_after_action
        .trigger();

    QCOMPARE(model.rowCount(), old_row_count + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_insert_into_data() { add_tables(); };

  void test_insert_into() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();
    select_cell(switch_table, 0, 0);
    song_editor.song_menu_bar.edit_menu.insert_menu.insert_into_start_action
        .trigger();

    QCOMPARE(model.rowCount(), old_row_count + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_interval_button_data() {
    auto &controls_column = song_editor.song_widget.controls_column;
    auto &third_row = controls_column.third_row;
    auto &fifth_row = controls_column.fifth_row;
    auto &seventh_row = controls_column.seventh_row;
    auto &octave_row = controls_column.octave_row;
    auto &third_minus_button = third_row.minus_button;
    auto &third_plus_button = third_row.plus_button;
    auto &fifth_minus_button = fifth_row.minus_button;
    auto &fifth_plus_button = fifth_row.plus_button;
    auto &seventh_minus_button = seventh_row.minus_button;
    auto &seventh_plus_button = seventh_row.plus_button;
    auto &octave_minus_button = octave_row.minus_button;
    auto &octave_plus_button = octave_row.plus_button;

    QTest::addColumn<QPushButton *>("button_pointer");
    add_table_columns();

    QTest::newRow("chord third -") << &third_minus_button << chord_type << -1;
    QTest::newRow("chord third +") << &third_plus_button << chord_type << -1;
    QTest::newRow("chord fifth -") << &fifth_minus_button << chord_type << -1;
    QTest::newRow("chord fifth +") << &fifth_plus_button << chord_type << -1;
    QTest::newRow("chord seventh -")
        << &seventh_minus_button << chord_type << -1;
    QTest::newRow("chord seventh +")
        << &seventh_plus_button << chord_type << -1;
    QTest::newRow("chord octave -") << &octave_minus_button << chord_type << -1;
    QTest::newRow("chord octave +") << &octave_plus_button << chord_type << -1;
    QTest::newRow("pitched note third -")
        << &third_minus_button << pitched_note_type << 1;
    QTest::newRow("pitched note third +")
        << &third_plus_button << pitched_note_type << 1;
    QTest::newRow("pitched note fifth -")
        << &fifth_minus_button << pitched_note_type << 1;
    QTest::newRow("pitched note fifth +")
        << &fifth_plus_button << pitched_note_type << 1;
    QTest::newRow("pitched note seventh -")
        << &seventh_minus_button << pitched_note_type << 1;
    QTest::newRow("pitched note seventh +")
        << &seventh_plus_button << pitched_note_type << 1;
    QTest::newRow("pitched note octave -")
        << &octave_minus_button << pitched_note_type << 1;
    QTest::newRow("pitched note octave +")
        << &octave_plus_button << pitched_note_type << 1;
  };

  void test_interval_button() {
    QFETCH(QPushButton *, button_pointer);
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);
    const auto test_index =
        get_model(switch_table)
            .index(0, row_type == chord_type
                          ? static_cast<int>(
                                static_cast<int>(chord_interval_column))
                          : static_cast<int>(pitched_note_interval_column));
    const auto original_data = test_index.data();
    get_selection_model(switch_table).select(test_index, SELECT_AND_CLEAR);

    get_reference(button_pointer).click();
    QCOMPARE_NE(original_data, test_index.data());
    undo_stack.undo();
    QCOMPARE(original_data, test_index.data());
    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_missing_instrument() {
    auto &song_widget = song_editor.song_widget;
    auto &song_menu_bar = song_editor.song_menu_bar;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &delete_cells_action = song_menu_bar.edit_menu.delete_cells_action;

    select_cell(switch_table, 1, chord_instrument_column);
    delete_cells_action.trigger();

    switch_to(switch_table, pitched_note_type, 1);
    select_cell(switch_table, 1, pitched_note_instrument_column);
    delete_cells_action.trigger();
    close_message_later(song_editor, waiting_for_message,
                        "No instrument for chord 2, pitched note 2");
    song_menu_bar.play_menu.play_action.trigger();

    // undo delete pitched_note instrument
    undo_stack.undo();
    maybe_switch_back_to_chords(undo_stack, pitched_note_type);
    // undo delete chord instrument
    undo_stack.undo();
  };

  void test_missing_percussion_set() {
    auto &song_widget = song_editor.song_widget;
    auto &song_menu_bar = song_editor.song_menu_bar;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &delete_cells_action = song_menu_bar.edit_menu.delete_cells_action;

    select_cell(switch_table, 1, chord_percussion_instrument_column);
    delete_cells_action.trigger();

    switch_to(switch_table, unpitched_note_type, 1);

    select_cell(switch_table, 1, unpitched_note_percussion_instrument_column);
    delete_cells_action.trigger();

    close_message_later(song_editor, waiting_for_message,
                        "No percussion set for chord 2, unpitched note 2");
    song_menu_bar.play_menu.play_action.trigger();

    // undo delete unpitched note percussion instrument
    undo_stack.undo();

    maybe_switch_back_to_chords(undo_stack, unpitched_note_type);
    // undo delete chord percussion instrument
    undo_stack.undo();
  };

  static void test_musicxml_data() {
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<int>("number_of_chords");

    QTest::newRow("prelude") << "prelude.musicxml" << MUSIC_XML_ROWS;
    QTest::newRow("percussion") << "percussion.musicxml" << PERCUSSION_ROWS;
  };

  void test_musicxml() {
    QFETCH(QString, file_name);
    QFETCH(int, number_of_chords);

    auto &song_widget = song_editor.song_widget;

    import_musicxml(song_widget, test_dir.filePath(file_name));
    QCOMPARE(get_model(song_widget.switch_column.switch_table)
                 .rowCount(QModelIndex()),
             number_of_chords);
    open_file(song_editor.song_widget, test_dir.filePath("test_song.xml"));
  };

  static void test_musicxml_error_data() {
    QTest::addColumn<QString>("file_name");
    QTest::addColumn<QString>("error_message");

    QTest::newRow("not musicxml")
        << "not_musicxml.xml" << "Invalid musicxml file";
    QTest::newRow("empty") << "empty.musicxml" << "No chords";
    QTest::newRow("grace notes") << "MozartPianoSonata.musicxml"
                                 << "Notes without durations not supported";
    QTest::newRow("transpositions")
        << "MozartTrio.musicxml" << "Transposition not supported";
    QTest::newRow("timewise")
        << "timewise.musicxml"
        << "Justly only supports partwise musicxml scores";
  };

  void test_musicxml_error() {
    QFETCH(QString, error_message);
    QFETCH(QString, file_name);

    close_message_later(song_editor, waiting_for_message, error_message);
    import_musicxml(song_editor.song_widget, test_dir.filePath(file_name));
  };

  static void test_next_previous_data() {
    add_table_columns();

    QTest::newRow("pitched note") << pitched_note_type << 1;
    QTest::newRow("unpitched note") << unpitched_note_type << 1;
  };

  void test_next_previous() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &view_menu = song_editor.song_menu_bar.view_menu;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_parent_chord_number(switch_table), chord_number);
    view_menu.previous_chord_action.trigger();
    QCOMPARE(get_parent_chord_number(switch_table), chord_number - 1);
    view_menu.next_chord_action.trigger();
    QCOMPARE(get_parent_chord_number(switch_table), chord_number);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  void test_octave_bound() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, pitched_note_type, 1);
    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message,
                        "Octave 10 (absolutely) greater than maximum 9");
    press_times(song_widget.controls_column.octave_row.plus_button,
                OCTAVE_SHIFT_TIMES + 1);
    undo_times(undo_stack, OCTAVE_SHIFT_TIMES); // undo shift octave

    maybe_switch_back_to_chords(undo_stack, pitched_note_type);
  };

  static void test_open_error_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("error_message");

    QTest::newRow("not xml") << "<" << "Invalid XML file";
    QTest::newRow("not Justly") << "<song/>" << "Invalid song file";
  };

  void test_open_error() {
    QFETCH(QString, text);
    QFETCH(QString, error_message);

    auto &song_widget = song_editor.song_widget;
    close_message_later(song_editor, waiting_for_message, error_message);
    open_text(song_widget, text);
  };

  static void test_paste_after_data() { add_cells(); };

  void test_paste_after() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, row_number);
    QFETCH(int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);
    select_cell(switch_table, row_number, column_number);
    edit_menu.copy_action.trigger();

    const auto number_of_rows = model.rowCount();
    edit_menu.paste_menu.paste_after_action.trigger();
    QCOMPARE(model.rowCount(), number_of_rows + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), number_of_rows);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_paste_error_data() {
    add_table_columns();
    QTest::addColumn<QString>("copied");
    QTest::addColumn<QString>("mime_type");
    QTest::addColumn<QString>("error_message");
    QTest::newRow("chord not a mime")
        << chord_type << -1 << "" << "not a mime"
        << "Cannot paste not a mime as chords cells";
    QTest::newRow("chord pitched notes mime")
        << chord_type << -1 << "" << PitchedNote::get_cells_mime()
        << "Cannot paste pitched notes cells as chords cells";
    QTest::newRow("chord not xml")
        << chord_type << -1 << "[" << Chord::get_cells_mime() << "Invalid XML";
    QTest::newRow("chord not Justly")
        << chord_type << -1 << "<song/>" << Chord::get_cells_mime()
        << "Invalid clipboard";
    QTest::newRow("pitched note not a mime")
        << pitched_note_type << 1 << "" << "not a mime"
        << "Cannot paste not a mime as pitched notes cells";
    QTest::newRow("pitched note chords mime")
        << pitched_note_type << 1 << "" << Chord::get_cells_mime()
        << "Cannot paste chords cells as pitched notes cells";
    QTest::newRow("pitched note not xml")
        << pitched_note_type << 1 << "<" << PitchedNote::get_cells_mime()
        << "Invalid XML";
    QTest::newRow("pitched note not Justly")
        << pitched_note_type << 1 << "<song/>" << PitchedNote::get_cells_mime()
        << "Invalid clipboard";
    QTest::newRow("unpitched note not a mime")
        << unpitched_note_type << 1 << "" << "not a mime"
        << "Cannot paste not a mime as unpitched notes cells";
    QTest::newRow("unpitched note chords mime")
        << unpitched_note_type << 1 << "" << Chord::get_cells_mime()
        << "Cannot paste chords cells as unpitched notes cells";
    QTest::newRow("unpitched note not xml")
        << unpitched_note_type << 1 << "<" << UnpitchedNote::get_cells_mime()
        << "Invalid XML";
    QTest::newRow("unpitched note not Justly")
        << unpitched_note_type << 1 << "<song/>"
        << UnpitchedNote::get_cells_mime() << "Invalid clipboard";
  };

  void test_paste_error() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(QString, copied);
    QFETCH(QString, mime_type);
    QFETCH(QString, error_message);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &new_data =
        get_reference(new QMimeData); // NOLINT(cppcoreguidelines-owning-memory)

    new_data.setData(mime_type, copied.toStdString().c_str());

    auto &clipboard = get_reference(QGuiApplication::clipboard());
    clipboard.setMimeData(&new_data);

    select_cell(switch_table, 0, 0);
    close_message_later(song_editor, waiting_for_message, error_message);
    song_editor.song_menu_bar.edit_menu.paste_menu.paste_over_action.trigger();

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_paste_into_data() { add_cells(); };

  void test_paste_into() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, row_number);
    QFETCH(int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;
    auto &edit_menu = song_editor.song_menu_bar.edit_menu;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);

    select_cell(switch_table, row_number, column_number);
    edit_menu.copy_action.trigger();

    const auto number_of_rows = model.rowCount();
    edit_menu.paste_menu.paste_into_start_action.trigger();
    QCOMPARE(model.rowCount(), number_of_rows + 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), number_of_rows);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_play_data() {

    add_table_columns();
    QTest::addColumn<int>("first_row_number");
    QTest::addColumn<int>("second_row_number");
    QTest::addColumn<int>("column_number");

    QTest::newRow("two chords") << chord_type << -1 << 0 << 1
                                << static_cast<int>(chord_interval_column);
    QTest::newRow("one chord") << chord_type << -1 << 1 << 1
                               << static_cast<int>(chord_interval_column);
    QTest::newRow("two pitched notes")
        << pitched_note_type << 1 << 0 << 1
        << static_cast<int>(pitched_note_instrument_column);
    QTest::newRow("one pitched note")
        << pitched_note_type << 1 << 1 << 1
        << static_cast<int>(pitched_note_instrument_column);
    QTest::newRow("two unpitched notes")
        << unpitched_note_type << 1 << 0 << 1
        << static_cast<int>(unpitched_note_percussion_instrument_column);
    QTest::newRow("one unpitched note")
        << unpitched_note_type << 1 << 1 << 1
        << static_cast<int>(unpitched_note_percussion_instrument_column);
  };

  void test_play() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, first_row_number);
    QFETCH(int, second_row_number);
    QFETCH(int, column_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &play_menu = song_editor.song_menu_bar.play_menu;
    auto &play_action = play_menu.play_action;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);
    get_selection_model(switch_table)
        .select(QItemSelection(model.index(first_row_number, column_number),
                               model.index(second_row_number, column_number)),
                SELECT_AND_CLEAR);
    play_action.trigger();
    // first cut off early
    play_action.trigger();
    // now play for a while
    QThread::msleep(WAIT_TIME);
    play_menu.stop_playing_action.trigger();

    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  void test_ratio_bound_data() {
    QTest::addColumn<QPushButton *>("fifth_button_pointer");
    QTest::addColumn<QPushButton *>("octave_button_pointer");
    QTest::addColumn<QString>("error_message");

    auto &controls_column = song_editor.song_widget.controls_column;
    auto &fifth_row = controls_column.fifth_row;
    auto &octave_row = controls_column.octave_row;

    QTest::newRow("too high")
        << &fifth_row.plus_button << &octave_row.plus_button
        << "Numerator 2187 greater than maximum 999";

    QTest::newRow("too low")
        << &fifth_row.minus_button << &octave_row.minus_button
        << "Denominator 2187 greater than maximum 999";
  }

  void test_ratio_bound() {
    QFETCH(QPushButton *, fifth_button_pointer);
    QFETCH(QPushButton *, octave_button_pointer);
    QFETCH(QString, error_message);

    auto &fifth_button = get_reference(fifth_button_pointer);
    auto &octave_button = get_reference(octave_button_pointer);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, pitched_note_type, 1);
    select_cell(switch_table, 0, 0);

    for (auto counter = 0; counter < RATIO_SHIFT_TIMES; counter++) {
      fifth_button.click();
      octave_button.click();
    }
    close_message_later(song_editor, waiting_for_message, error_message);
    fifth_button.click();
    undo_times(undo_stack, RATIO_SHIFT_TIMES * 2); // undo shift numerator

    maybe_switch_back_to_chords(undo_stack, pitched_note_type);
  };

  static void test_remove_row_data() { add_tables(); };

  void test_remove_row() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);

    const auto old_row_count = model.rowCount();

    select_cell(switch_table, 0, 0);
    song_editor.song_menu_bar.edit_menu.remove_rows_action.trigger();

    QCOMPARE(model.rowCount(), old_row_count - 1);
    undo_stack.undo();
    QCOMPARE(model.rowCount(), old_row_count);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  void test_replace_table_combining() {
    auto &song_widget = song_editor.song_widget;
    switch_to(song_widget.switch_column.switch_table, unpitched_note_type, 0);
    song_editor.song_menu_bar.view_menu.back_to_chords_action.trigger();
    QVERIFY(!song_widget.undo_stack.canUndo());
  };

  static void test_row_count_data() {
    add_table_columns();
    QTest::addColumn<int>("number_of_rows");

    QTest::newRow("chords") << chord_type << -1 << EIGHT;
    QTest::newRow("chord 0 pitched notes") << pitched_note_type << 0 << 0;
    QTest::newRow("chord 1 pitched notes") << pitched_note_type << 1 << EIGHT;
    QTest::newRow("chord 2 pitched notes") << pitched_note_type << 2 << 0;
    QTest::newRow("chord 3 pitched notes") << pitched_note_type << 3 << 0;
    QTest::newRow("chord 4 pitched notes") << pitched_note_type << 4 << 0;
    QTest::newRow("chord 5 pitched notes") << pitched_note_type << FIVE << 0;
    QTest::newRow("chord 6 pitched notes") << pitched_note_type << SIX << 0;
    QTest::newRow("chord 7 pitched notes") << pitched_note_type << SEVEN << 0;
    QTest::newRow("chord 0 unpitched notes") << unpitched_note_type << 0 << 0;
    QTest::newRow("chord 1 unpitched notes") << unpitched_note_type << 1 << 4;
    QTest::newRow("chord 2 unpitched notes") << unpitched_note_type << 2 << 0;
    QTest::newRow("chord 3 unpitched notes") << unpitched_note_type << 3 << 0;
    QTest::newRow("chord 4 unpitched notes") << unpitched_note_type << 4 << 0;
    QTest::newRow("chord 5 unpitched notes")
        << unpitched_note_type << FIVE << 0;
    QTest::newRow("chord 6 unpitched notes") << unpitched_note_type << SIX << 0;
    QTest::newRow("chord 7 unpitched notes")
        << unpitched_note_type << SEVEN << 0;
  };

  void test_row_count() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, number_of_rows);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_model(switch_table).rowCount(), number_of_rows);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_row_header_data() {
    QTest::addColumn<Qt::ItemDataRole>("role");
    QTest::addColumn<QVariant>("data");

    QTest::newRow("text") << Qt::DisplayRole << QVariant(1);
    QTest::newRow("unused role") << Qt::DecorationRole << QVariant();
  };

  void test_row_header() {
    QFETCH(Qt::ItemDataRole, role);
    QFETCH(QVariant, data);

    QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
                 .headerData(0, Qt::Vertical, role),
             data);
  };

  void test_save() {
    auto &song_widget = song_editor.song_widget;
    auto &song_menu_bar = song_editor.song_menu_bar;

    auto original_text = get_file_text(test_dir.filePath("test_song.xml"));

    // save into a temp file
    auto save_filename = test_dir.filePath("test_song_2.xml");
    save_as_file(song_widget, save_filename);
    QCOMPARE(song_widget.current_file, save_filename);

    // compare the saved text to the original text
    QCOMPARE(original_text, get_file_text(save_filename));

    // now change the song and save to the same file
    song_widget.controls_column.spin_boxes.gain_editor.setValue(NEW_GAIN_1);
    song_menu_bar.file_menu.save_action.trigger();
    song_widget.undo_stack.undo();

    QCOMPARE_NE(original_text, get_file_text(save_filename));

    QFile(save_filename).remove();
  };

  void test_starting_control_data() {
    QTest::addColumn<QDoubleSpinBox *>("spin_box_pointer");
    QTest::addColumn<double *>("value_pointer");
    QTest::addColumn<double>("first_value");
    QTest::addColumn<double>("second_value");

    auto &song_widget = song_editor.song_widget;
    auto &song = song_widget.song;
    auto &spin_boxes = song_widget.controls_column.spin_boxes;

    QTest::newRow("key") << &spin_boxes.starting_key_editor
                         << &song.starting_key << STARTING_KEY_1
                         << STARTING_KEY_2;
    QTest::newRow("velocity")
        << &spin_boxes.starting_velocity_editor << &song.starting_velocity
        << STARTING_VELOCITY_1 << STARTING_VELOCITY_2;
    QTest::newRow("tempo") << &spin_boxes.starting_tempo_editor
                           << &song.starting_tempo << STARTING_TEMPO_1
                           << STARTING_TEMPO_2;
  }

  void test_starting_control() {
    QFETCH(QDoubleSpinBox *, spin_box_pointer);
    QFETCH(double *, value_pointer);
    QFETCH(double, first_value);
    QFETCH(double, second_value);

    auto &spin_box = get_reference(spin_box_pointer);
    auto &value = get_reference(value_pointer);

    const auto old_value = value;
    QCOMPARE_NE(old_value, first_value);
    QCOMPARE_NE(old_value, second_value);

    // test combining
    spin_box.setValue(first_value);
    QCOMPARE(value, first_value);
    spin_box.setValue(second_value);
    QCOMPARE(value, second_value);
    song_editor.song_widget.undo_stack.undo();
    QCOMPARE(value, old_value);
  };

  static void test_status_data() {
    add_table_columns();
    QTest::addColumn<QString>("status");

    QTest::newRow("pitched note") << pitched_note_type << 1
                                  << "660 Hz ≈ E5 + 2 cents; Velocity 30; 300 "
                                     "bpm; Start at 600 ms; Duration 200 ms";
    QTest::newRow("unpitched note")
        << unpitched_note_type << 1
        << "Velocity 30; 300 bpm; Start at 600 ms; Duration 200 ms";
  }

  void test_status() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(QString, status);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    QCOMPARE(get_model(switch_table).index(0, 0).data(Qt::StatusTipRole),
             status);
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };

  static void test_set_value_data() { add_editable_cell_pairs(); };

  void test_set_value() {
    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;
    auto &undo_stack = song_widget.undo_stack;

    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);
    QFETCH(int, first_row_number);
    QFETCH(int, second_row_number);
    QFETCH(int, column_number);

    switch_to(switch_table, row_type, chord_number);

    auto &model = get_model(switch_table);
    const auto second_index = model.index(second_row_number, column_number);

    const auto first_value =
        model.index(first_row_number, column_number).data();
    const auto second_value = second_index.data();
    QCOMPARE_NE(first_value, second_value);

    auto &delegate = get_reference(switch_table.itemDelegate());
    auto &cell_editor = get_reference(
        delegate.createEditor(&get_reference(switch_table.viewport()),
                              QStyleOptionViewItem(), second_index));
    delegate.setEditorData(&cell_editor, second_index);
    cell_editor.setProperty(
        get_reference(cell_editor.metaObject()).userProperty().name(),
        first_value);
    delegate.setModelData(&cell_editor, &model, second_index);

    QCOMPARE(second_index.data(), first_value);
    undo_stack.undo();
    QCOMPARE(second_index.data(), second_value);

    maybe_switch_back_to_chords(undo_stack, row_type);
  };

  static void test_to_string_data() {
    QTest::addColumn<int>("row_number");
    QTest::addColumn<int>("column_number");
    QTest::addColumn<QString>("text");

    QTest::newRow("chord 0 instrument")
        << 0 << static_cast<int>(chord_instrument_column) << "";
    QTest::newRow("chord 0 percussion instrument")
        << 0 << static_cast<int>(chord_percussion_instrument_column) << "";
    QTest::newRow("chord 0 interval")
        << 0 << static_cast<int>(chord_interval_column) << "";
    QTest::newRow("chord 1 interval")
        << 1 << static_cast<int>(chord_interval_column) << "3";
    QTest::newRow("chord 2 interval")
        << 2 << static_cast<int>(chord_interval_column) << "/5";
    QTest::newRow("chord 3 interval")
        << 3 << static_cast<int>(chord_interval_column) << "3/5";
    QTest::newRow("chord 4 interval")
        << 4 << static_cast<int>(chord_interval_column) << "o1";
    QTest::newRow("chord 5 interval")
        << FIVE << static_cast<int>(chord_interval_column) << "3o1";
    QTest::newRow("chord 6 interval")
        << SIX << static_cast<int>(chord_interval_column) << "/5o1";
    QTest::newRow("chord 7 interval")
        << SEVEN << static_cast<int>(chord_interval_column) << "3/5o1";
    QTest::newRow("chord 0 beats")
        << 0 << static_cast<int>(chord_beats_column) << "";
    QTest::newRow("chord 1 beats")
        << 1 << static_cast<int>(chord_beats_column) << "3";
    QTest::newRow("chord 2 beats")
        << 2 << static_cast<int>(chord_beats_column) << "/5";
    QTest::newRow("chord 3 beats")
        << 3 << static_cast<int>(chord_beats_column) << "3/5";
    QTest::newRow("chord 1 percussion instrument")
        << 1 << static_cast<int>(chord_percussion_instrument_column)
        << "Standard #35";
  };

  void test_to_string() {
    QFETCH(int, row_number);
    QFETCH(int, column_number);
    QFETCH(QString, text);

    QCOMPARE(get_model(song_editor.song_widget.switch_column.switch_table)
                 .index(row_number, column_number)
                 .data()
                 .toString(),
             text);
  };

  static void test_unused_role_data() { add_tables(); };

  void test_unused_role() {
    QFETCH(RowType, row_type);
    QFETCH(int, chord_number);

    auto &song_widget = song_editor.song_widget;
    auto &switch_table = song_widget.switch_column.switch_table;

    switch_to(switch_table, row_type, chord_number);
    auto &model = get_model(switch_table);
    const auto test_index = model.index(0, 0);
    QCOMPARE(test_index.data(Qt::DecorationRole), QVariant());
    QVERIFY(!(model.setData(test_index, QVariant(), Qt::DecorationRole)));
    maybe_switch_back_to_chords(song_widget.undo_stack, row_type);
  };
};