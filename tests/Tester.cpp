#include "tests/Tester.h"

#include <qabstractitemmodel.h>  // for QModelIndex
#include <qapplication.h>        // for QApplication
#include <qdebug.h>              // for operator<<
#include <qflags.h>              // for QFlags, operator==, QFlags<>:...
#include <qlist.h>               // for QList, QList<>::iterator
#include <qlogging.h>            // for QtWarningMsg
#include <qmessagebox.h>         // for QMessageBox
#include <qmetaobject.h>         // for QMetaProperty
#include <qnamespace.h>          // for qt_getEnumName, ItemDataRole
#include <qobject.h>             // for qobject_cast
#include <qobjectdefs.h>         // for QMetaObject
#include <qstring.h>             // for QString
#include <qtemporaryfile.h>      // for QTemporaryFile
#include <qtest.h>               // for qCompare
#include <qtestcase.h>           // for newRow, qCompare, QCOMPARE
#include <qtestdata.h>           // for operator<<, QTestData
#include <qtestkeyboard.h>       // for keyEvent, Press
#include <qthread.h>             // for QThread
#include <qtimer.h>              // for QTimer
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget
#include <qwindowdefs.h>         // for QWidgetList

#include <memory>       // for allocator, make_unique, __uni...
#include <type_traits>  // for enable_if_t

#include "justly/Instrument.hpp"        // for get_instrument
#include "justly/Interval.hpp"          // for Interval
#include "justly/NoteChordField.hpp"    // for interval_column, beats_column
#include "justly/Rational.hpp"          // for Rational
#include "justly/SongEditor.hpp"        // for SongEditor
#include "justly/public_constants.hpp"  // for DEFAULT_COLOR, NON_DEFAULT_COLOR

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
      QTest::keyEvent(QTest::Press, box_pointer,
                      Qt::Key_Enter);
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
  song_editor.open_file(main_file.fileName());

  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  QCOMPARE(song_editor.get_number_of_children(1), 1);
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
}

void Tester::test_playback_volume_control() {
  song_editor.set_playback_volume(1);
  QCOMPARE(song_editor.get_playback_volume(), 1);
}

void Tester::test_starting_instrument_control() {
  const auto *original_value = get_instrument_pointer("Marimba");
  const auto *new_value = get_instrument_pointer("Oboe");
  const auto *new_value_2 = get_instrument_pointer("Ocarina");

  const auto *old_value = song_editor.get_starting_instrument();
  QCOMPARE(old_value, get_instrument_pointer("Marimba"));

  // test change
  song_editor.set_starting_instrument_undoable(new_value);
  QCOMPARE(song_editor.get_starting_instrument(), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_instrument(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_starting_instrument(), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_instrument(), original_value);

  // test combining
  song_editor.set_starting_instrument_undoable(new_value);
  song_editor.set_starting_instrument_undoable(new_value_2);
  QCOMPARE(song_editor.get_starting_instrument(), new_value_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_instrument(), original_value);
}

void Tester::test_starting_key_control() {
  auto old_value = song_editor.get_starting_key();
  QCOMPARE(old_value, ORIGINAL_KEY);

  // test change
  song_editor.set_starting_key_undoable(STARTING_KEY_1);
  QCOMPARE(song_editor.get_starting_key(), STARTING_KEY_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_key(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_starting_key(), STARTING_KEY_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_key(), ORIGINAL_KEY);

  // test combining
  song_editor.set_starting_key_undoable(STARTING_KEY_1);
  song_editor.set_starting_key_undoable(STARTING_KEY_2);
  QCOMPARE(song_editor.get_starting_key(), STARTING_KEY_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_key(), ORIGINAL_KEY);
}

void Tester::test_starting_volume_control() {
  auto old_value = song_editor.get_starting_volume();
  QCOMPARE(old_value, ORIGINAL_VOLUME);

  // test change
  song_editor.set_starting_volume_undoable(STARTING_VOLUME_1);
  QCOMPARE(song_editor.get_starting_volume(), STARTING_VOLUME_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_volume(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_starting_volume(), STARTING_VOLUME_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_volume(), ORIGINAL_VOLUME);

  // test combining
  song_editor.set_starting_volume_undoable(STARTING_VOLUME_1);
  song_editor.set_starting_volume_undoable(STARTING_VOLUME_2);
  QCOMPARE(song_editor.get_starting_volume(), STARTING_VOLUME_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_volume(), ORIGINAL_VOLUME);
}

void Tester::test_starting_tempo_control() {
  auto old_value = song_editor.get_starting_tempo();
  QCOMPARE(old_value, ORIGINAL_TEMPO);

  // test change
  song_editor.set_starting_tempo_undoable(STARTING_TEMPO_1);
  QCOMPARE(song_editor.get_starting_tempo(), STARTING_TEMPO_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_tempo(), old_value);

  // test redo
  song_editor.redo();
  QCOMPARE(song_editor.get_starting_tempo(), STARTING_TEMPO_1);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_tempo(), ORIGINAL_TEMPO);

  // test combining
  song_editor.set_starting_tempo_undoable(STARTING_TEMPO_1);
  song_editor.set_starting_tempo_undoable(STARTING_TEMPO_2);
  QCOMPARE(song_editor.get_starting_tempo(), STARTING_TEMPO_2);
  song_editor.undo();
  QCOMPARE(song_editor.get_starting_tempo(), ORIGINAL_TEMPO);
}

void Tester::test_tree() {
  // test song
  QCOMPARE(song_editor.get_row_count(song_editor.get_index()), 3);
  QCOMPARE(song_editor.get_column_count(QModelIndex()), NOTE_CHORD_COLUMNS);

  QCOMPARE(song_editor.get_parent_index(song_editor.get_index(0)),
           song_editor.get_index());
  // only nest the symbol column
  QCOMPARE(
      song_editor.get_row_count(song_editor.get_index(0, -1, interval_column)),
      0);

  // test first note
  QCOMPARE(song_editor.get_parent_index(song_editor.get_index(0, 0)).row(), 0);

  // QCOMPARE(song_editor.size_hint_for_column(-1), 0);
}

void Tester::test_copy_paste() {
  song_editor.select_index(song_editor.get_index(0));
  song_editor.copy_selected();
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0));
  song_editor.paste_before();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0));
  song_editor.paste_after();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.copy_selected();
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.paste_before();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.paste_after();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(2));
  song_editor.paste_into();
  QCOMPARE(song_editor.get_number_of_children(2), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(2), 0);
  song_editor.clear_selection();

  song_editor.paste_text(0, "[", song_editor.get_index());

  song_editor.paste_text(0, "{}", song_editor.get_index());

  song_editor.paste_text(0, "[", song_editor.get_index(0));

  song_editor.paste_text(0, "{}", song_editor.get_index(0));
}

void Tester::test_insert_delete() {
  song_editor.select_index(song_editor.get_index(2));
  song_editor.insert_into();
  QCOMPARE(song_editor.get_number_of_children(2), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(2), 0);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.insert_before();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_number_of_children(0), 3);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0, 0));
  song_editor.remove_selected();
  QCOMPARE(song_editor.get_number_of_children(0), 1);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(0), 2);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0));
  song_editor.insert_before();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0));
  song_editor.remove_selected();
  QCOMPARE(song_editor.get_number_of_children(-1), 2);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  song_editor.clear_selection();

  song_editor.select_index(song_editor.get_index(0));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_number_of_children(-1), 4);
  song_editor.undo();
  QCOMPARE(song_editor.get_number_of_children(-1), 3);
  song_editor.clear_selection();

  // test chord templating from previous chord
  song_editor.select_index(song_editor.get_index(1));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_data(song_editor.get_index(1, -1, beats_column),
                                Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  song_editor.undo();
  song_editor.clear_selection();

  // test note templating from previous note
  song_editor.select_index(song_editor.get_index(0, 1));
  song_editor.insert_after();
  QCOMPARE(song_editor.get_data(song_editor.get_index(0, 2, beats_column),
                                Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.get_data(
               song_editor.get_index(0, 2, volume_ratio_column), Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.get_data(song_editor.get_index(0, 2, tempo_ratio_column),
                                Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.get_data(song_editor.get_index(0, 2, words_column),
                                Qt::EditRole),
           "hello");
  song_editor.undo();
  song_editor.clear_selection();

  // test note inheritance from chord
  song_editor.select_index(song_editor.get_index(1));
  song_editor.insert_into();
  QCOMPARE(song_editor.get_data(song_editor.get_index(1, 0, beats_column),
                                Qt::EditRole),
           QVariant::fromValue(Rational(2)));
  QCOMPARE(song_editor.get_data(song_editor.get_index(1, 0, words_column),
                                Qt::EditRole),
           "hello");
  song_editor.undo();
  song_editor.clear_selection();
}

void Tester::test_column_headers_template() const {
  QFETCH(const NoteChordField, field);
  QFETCH(const QVariant, value);

  QCOMPARE(song_editor.get_header_data(field, Qt::Horizontal, Qt::DisplayRole),
           value);
}

void Tester::test_column_headers_template_data() {
  QTest::addColumn<NoteChordField>("field");
  QTest::addColumn<Qt::Orientation>("orientation");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");

  QTest::newRow("symbol header")
      << symbol_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant();
  QTest::newRow("interval header")
      << interval_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant("Interval");
  QTest::newRow("beats header")
      << beats_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant("Beats");
  QTest::newRow("volume header")
      << volume_ratio_column << Qt::Horizontal
      << Qt::DisplayRole << QVariant("Volume ratio");
  QTest::newRow("tempo header")
      << tempo_ratio_column << Qt::Horizontal
      << Qt::DisplayRole << QVariant("Tempo ratio");
  QTest::newRow("words header")
      << words_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant("Words");
  QTest::newRow("instruments header")
      << instrument_column << Qt::Horizontal
      << Qt::DisplayRole << QVariant("Instrument");
  QTest::newRow("horizontal labels")
      << symbol_column << Qt::Horizontal << Qt::DisplayRole
      << QVariant();
  QTest::newRow("wrong role")
      << symbol_column << Qt::Horizontal << Qt::DecorationRole
      << QVariant();
  // QTest::newRow("non-existent column")
  //    << -1 << Qt::Horizontal << Qt::DecorationRole << QVariant();
}

void Tester::test_select_template() {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, second_index);
  song_editor.select_index(first_index);
  song_editor.select_index(second_index);
  auto selected_rows = song_editor.get_selected_rows();
  QCOMPARE(selected_rows.size(), 1);
  QCOMPARE(selected_rows[0], first_index);
  song_editor.clear_selection();
}

void Tester::test_select_template_data() {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("second_index");

  QTest::newRow("select_chord_then_note")
      << song_editor.get_index(0) << song_editor.get_index(0, 0);
  QTest::newRow("select_note_then_chord")
      << song_editor.get_index(0, 0) << song_editor.get_index(0);
}

void Tester::test_flags_template() const {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemFlags, item_flags);
  QCOMPARE(song_editor.get_flags(index), item_flags);
}

void Tester::test_flags_template_data() const {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemFlags>("item_flags");

  QTest::newRow("first_chord_symbol_flag")
      << song_editor.get_index(0) << (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QTest::newRow("first_chord_interval_flag")
      << song_editor.get_index(0, -1, interval_column)
      << (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
}

void Tester::test_colors_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const bool, non_default);
  QCOMPARE(song_editor.get_data(index, Qt::ForegroundRole),
           non_default ? NON_DEFAULT_COLOR : DEFAULT_COLOR);
}

void Tester::test_colors_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<bool>("non_default");

  QTest::newRow("first_chord_symbol_color") << song_editor.get_index(0) << true;
  QTest::newRow("first_chord_interval_color")
      << song_editor.get_index(0, -1, interval_column) << false;
  QTest::newRow("first_chord_beats_color")
      << song_editor.get_index(0, -1, beats_column) << false;
  QTest::newRow("first_chord_volume_color")
      << song_editor.get_index(0, -1, volume_ratio_column) << false;
  QTest::newRow("first_chord_tempo_color")
      << song_editor.get_index(0, -1, tempo_ratio_column) << false;
  QTest::newRow("first_chord_words_color")
      << song_editor.get_index(0, -1, words_column) << false;
  QTest::newRow("first_chord_instrument_color")
      << song_editor.get_index(0, -1, instrument_column) << false;

  QTest::newRow("second_chord_interval_color")
      << song_editor.get_index(1, -1, interval_column) << true;
  QTest::newRow("second_chord_beats_color")
      << song_editor.get_index(1, -1, beats_column) << true;
  QTest::newRow("second_chord_volume_color")
      << song_editor.get_index(1, -1, volume_ratio_column) << true;
  QTest::newRow("second_chord_tempo_color")
      << song_editor.get_index(1, -1, tempo_ratio_column) << true;
  QTest::newRow("second_chord_words_color")
      << song_editor.get_index(1, -1, words_column) << true;

  QTest::newRow("first_note_symbol_color")
      << song_editor.get_index(0, 0) << true;
  QTest::newRow("first_note_interval_color")
      << song_editor.get_index(0, 0, interval_column) << false;
  QTest::newRow("first_note_beats_color")
      << song_editor.get_index(0, 0, beats_column) << false;
  QTest::newRow("first_note_volume_color")
      << song_editor.get_index(0, 0, volume_ratio_column) << false;
  QTest::newRow("first_note_tempo_color")
      << song_editor.get_index(0, 0, tempo_ratio_column) << false;
  QTest::newRow("first_note_words_color")
      << song_editor.get_index(0, 0, words_column) << false;
  QTest::newRow("first_note_instrument_color")
      << song_editor.get_index(0, 0, instrument_column) << false;

  QTest::newRow("second_note_interval_color")
      << song_editor.get_index(0, 1, interval_column) << true;
  QTest::newRow("second_note_beats_color")
      << song_editor.get_index(0, 1, beats_column) << true;
  QTest::newRow("second_note_volume_color")
      << song_editor.get_index(0, 1, volume_ratio_column) << true;
  QTest::newRow("second_note_tempo_color")
      << song_editor.get_index(0, 1, tempo_ratio_column) << true;
  QTest::newRow("second_note_words_color")
      << song_editor.get_index(0, 1, words_column) << true;
  QTest::newRow("second_note_instrument_color")
      << song_editor.get_index(0, 1, instrument_column) << true;
}

void Tester::test_get_value_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const Qt::ItemDataRole, role);
  QFETCH(const QVariant, value);
  QCOMPARE(song_editor.get_data(index, role), value);
}

void Tester::test_get_value_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<Qt::ItemDataRole>("role");
  QTest::addColumn<QVariant>("value");
  QTest::newRow("first_chord_symbol")
      << song_editor.get_index(0) << Qt::DisplayRole << QVariant("♫");
  QTest::newRow("first_chord_decoration")
      << song_editor.get_index(0) << Qt::DecorationRole << QVariant();
  QTest::newRow("first_note_symbol")
      << song_editor.get_index(0, 0) << Qt::DisplayRole << QVariant("♪");
  QTest::newRow("first_note_decoration")
      << song_editor.get_index(0, 0) << Qt::DecorationRole << QVariant();
  QTest::newRow("second_note_interval")
      << song_editor.get_index(0, 1, interval_column) << Qt::DisplayRole
      << QVariant("2/2o1");
}

void Tester::test_delegate_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, new_value);

  auto *cell_editor_pointer = song_editor.create_editor(index);

  QCOMPARE(cell_editor_pointer->property(
               cell_editor_pointer->metaObject()->userProperty().name()),
           old_value);

  song_editor.set_editor(cell_editor_pointer, index, new_value);

  QCOMPARE(song_editor.get_data(index, Qt::EditRole), new_value);
  song_editor.undo();
  QCOMPARE(song_editor.get_data(index, Qt::EditRole), old_value);
}

void Tester::test_delegate_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("new_value");

  QTest::newRow("instrument editor")
      << song_editor.get_index(0, -1, instrument_column)
      << QVariant::fromValue(get_instrument_pointer(""))
      << QVariant::fromValue(get_instrument_pointer("Oboe"));
  QTest::newRow("interval editor")
      << song_editor.get_index(0, -1, interval_column)
      << QVariant::fromValue(Interval(1)) << QVariant::fromValue(Interval(2));
  QTest::newRow("beats editor")
      << song_editor.get_index(0, -1, beats_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("volume editor")
      << song_editor.get_index(0, -1, volume_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("tempo editor")
      << song_editor.get_index(0, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational(1)) << QVariant::fromValue(Rational(2));
  QTest::newRow("words editor") << song_editor.get_index(0, -1, words_column)
                                << QVariant("") << QVariant("hello");
}

void Tester::test_set_value() {
  QVERIFY(
      song_editor.set_data(song_editor.get_index(0), QVariant(), Qt::EditRole));
  // setData only works for the edit role
  QVERIFY(!(song_editor.set_data(song_editor.get_index(0), QVariant(),
                                 Qt::DecorationRole)));

  // test undo merging
  auto new_interval = QVariant::fromValue(Interval(3, 2));
  auto first_chord_index = song_editor.get_index(0, -1, interval_column);
  QVERIFY(song_editor.set_data(
      first_chord_index, QVariant::fromValue(Interval(5, 4)), Qt::EditRole));
  QVERIFY(song_editor.set_data(first_chord_index, new_interval, Qt::EditRole));
  QCOMPARE(song_editor.get_data(first_chord_index, Qt::EditRole), new_interval);
  song_editor.undo();
  QCOMPARE(song_editor.get_data(first_chord_index, Qt::EditRole),
           QVariant::fromValue(Interval()));
}

void Tester::test_set_value_template() {
  QFETCH(const QModelIndex, index);
  QFETCH(const QVariant, old_value);
  QFETCH(const QVariant, old_display_value);
  QFETCH(const QVariant, new_value);
  QFETCH(const QVariant, new_display_value);

  QVERIFY(song_editor.set_data(index, new_value, Qt::EditRole));

  QCOMPARE(song_editor.get_data(index, Qt::EditRole), new_value);
  QCOMPARE(song_editor.get_data(index, Qt::DisplayRole), new_display_value);

  song_editor.undo();
  song_editor.redo();
  song_editor.undo();

  QCOMPARE(song_editor.get_data(index, Qt::EditRole), old_value);
  QCOMPARE(song_editor.get_data(index, Qt::DisplayRole), old_display_value);
}

void Tester::test_set_value_template_data() {
  QTest::addColumn<QModelIndex>("index");
  QTest::addColumn<QVariant>("old_value");
  QTest::addColumn<QVariant>("old_display_value");
  QTest::addColumn<QVariant>("new_value");
  QTest::addColumn<QVariant>("new_display_value");

  QTest::newRow("first_chord_interval")
      << song_editor.get_index(0, -1, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_chord_beats")
      << song_editor.get_index(0, -1, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_volume")
      << song_editor.get_index(0, -1, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_tempo")
      << song_editor.get_index(0, -1, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_chord_words")
      << song_editor.get_index(0, -1, words_column) << QVariant("")
      << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_chord_instrument")
      << song_editor.get_index(0, -1, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe")) << QVariant("Oboe");
  QTest::newRow("first_note_interval")
      << song_editor.get_index(0, 0, interval_column)
      << QVariant::fromValue(Interval()) << QVariant("1")
      << QVariant::fromValue(Interval(2)) << QVariant("2");
  QTest::newRow("first_note_beats")
      << song_editor.get_index(0, 0, beats_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_volume")
      << song_editor.get_index(0, 0, volume_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_tempo")
      << song_editor.get_index(0, 0, tempo_ratio_column)
      << QVariant::fromValue(Rational()) << QVariant("1")
      << QVariant::fromValue(Rational(2)) << QVariant("2");
  QTest::newRow("first_note_words")
      << song_editor.get_index(0, 0, words_column) << QVariant("")
      << QVariant("") << QVariant("hello") << QVariant("hello");
  QTest::newRow("first_note_instrument")
      << song_editor.get_index(0, 0, instrument_column)
      << QVariant::fromValue(get_instrument_pointer("")) << QVariant("")
      << QVariant::fromValue(get_instrument_pointer("Oboe")) << QVariant("Oboe");
}

void Tester::test_io() {
  QTemporaryFile temp_json_file;
  temp_json_file.open();
  temp_json_file.close();
  song_editor.save_as_file(temp_json_file.fileName());
  QCOMPARE(song_editor.get_current_file(), temp_json_file.fileName());
  song_editor.save();

  const QTemporaryFile temp_wav_file;
  temp_json_file.open();
  temp_json_file.close();
  song_editor.export_to(temp_json_file.fileName().toStdString());

  const QTemporaryFile broken_json_file;
  temp_json_file.open();
  temp_json_file.write("{");
  temp_json_file.close();
  song_editor.open_file(broken_json_file.fileName());
}

void Tester::test_play() {
  // Test volume errors
  song_editor.select_index(song_editor.get_index(0, 0, volume_ratio_column));
  QVERIFY(song_editor.set_data(song_editor.get_index(0, 0, volume_ratio_column),
                               QVariant::fromValue(Rational(10)),
                               Qt::EditRole));
  QTimer *const timer_pointer = std::make_unique<QTimer>(this).release();
  connect(timer_pointer, &QTimer::timeout, this, &close_message);
  timer_pointer->start(WAIT_TIME);
  song_editor.play_selected();
  song_editor.stop_playing();
  song_editor.undo();

  if (song_editor.has_real_time()) {
      QTest::ignoreMessage(QtWarningMsg,
                       "Cannot start audio driver \"not a driver\"");
    song_editor.start_real_time("not a driver");
    song_editor.start_real_time();
  }

  // Test midi overload
  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    song_editor.select_index(song_editor.get_index(0, 0));
    song_editor.insert_before();
    song_editor.clear_selection();
  }

  QTimer *const timer_pointer_2 = std::make_unique<QTimer>(this).release();
  connect(timer_pointer_2, &QTimer::timeout, this, &close_message);
  timer_pointer_2->start(WAIT_TIME);

  song_editor.select_index(song_editor.get_index(0));
  song_editor.play_selected();
  song_editor.stop_playing();
  song_editor.clear_selection();

  for (auto index = 0; index < OVERLOAD_NUMBER; index = index + 1) {
    song_editor.undo();
  }
}

void Tester::test_play_template() {
  QFETCH(const QModelIndex, first_index);
  QFETCH(const QModelIndex, last_index);

  song_editor.select_indices(first_index, last_index);
  // use the second chord to test key changing
  song_editor.play_selected();
  // first cut off early
  song_editor.play_selected();
  // now play the whole thing
  QThread::msleep(WAIT_TIME);
  song_editor.clear_selection();
}

void Tester::test_play_template_data() const {
  QTest::addColumn<QModelIndex>("first_index");
  QTest::addColumn<QModelIndex>("last_index");

  QTest::newRow("first two chords")
      << song_editor.get_index(0) << song_editor.get_index(1);
  QTest::newRow("second chord")
      << song_editor.get_index(1) << song_editor.get_index(1);

  auto first_chord_second_note_index = song_editor.get_index(0, 1);
  QTest::newRow("first chord second note")
      << first_chord_second_note_index << first_chord_second_note_index;

  auto second_chord_first_note_index = song_editor.get_index(1, 0);
  QTest::newRow("first note")
      << second_chord_first_note_index << second_chord_first_note_index;
}
