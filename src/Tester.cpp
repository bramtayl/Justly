#include "Tester.h"

#include <QtCore/qglobal.h>      // for QFlags
#include <qabstractitemmodel.h>  // for QModelIndex
#include <qjsonarray.h>          // for QJsonArray
#include <qjsonobject.h>         // for QJsonObject
#include <qjsonvalue.h>          // for QJsonValueRef
#include <qnamespace.h>          // for DisplayRole, operator|, Horizontal
#include <qtestcase.h>           // for qCompare, QCOMPARE, QVERIFY
#include <qundostack.h>          // for QUndoStack
#include <qvariant.h>            // for QVariant

#include <QModelIndexList>
#include <QTemporaryFile>
#include <memory>  // for unique_ptr
#include <thread>  // for sleep_for

#include "Chord.h"      // for CHORD_LEVEL
#include "Note.h"       // for NOTE_LEVEL
#include "NoteChord.h"  // for NoteChord, numerator_column, symbol_...
#include "Song.h"       // for Song, NOTE_CHORD_COLUMNS
#include "TreeNode.h"   // for TreeNode, ROOT_LEVEL
#include "Utilities.h"

const auto NON_EXISTENT_COLUMN = -1;

const auto TWO_DOUBLE = 2.0;

const auto WAIT_TIME = 3000;

const auto LIGHT_GRAY = QVariant(QColor(Qt::lightGray));

const auto NO_DATA = QVariant();

auto Tester::get_column_heading(int column) const -> QVariant {
  return editor.song.headerData(column, Qt::Horizontal, Qt::DisplayRole);
}

auto Tester::get_data(int row, int column, QModelIndex &parent_index,
                      Qt::ItemDataRole role) -> QVariant {
  auto &song = editor.song;
  return song.data(song.index(row, column, parent_index), role);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) -> bool {
  auto &song = editor.song;
  return song.setData(song.index(row, column, parent_index), new_value,
                 Qt::EditRole);
}

void Tester::initTestCase() {
  if (test_file.open()) {
    QTextStream test_io(&test_file);
    test_io << R""""(
{
    "children": [
        {
            "children": [
                {
                },
                {
                    "numerator": 2,
                    "denominator": 2,
                    "octave": 1,
                    "beats": 2,
                    "volume_percent": 2.0,
                    "tempo_percent": 2.0,
                    "words": "hello",
                    "instrument": "Wurley"
                }
            ]
        },
        {
            "numerator": 2,
            "denominator": 2,
            "octave": 1,
            "beats": 2,
            "volume_percent": 2.0,
            "tempo_percent": 2.0,
            "words": "hello",
            "children": []
        }
    ],
    "default_instrument": "Plucked",
    "frequency": 220,
    "orchestra_text": "\nnchnls = 2\n0dbfs = 1\ninstr BandedWG\n    a_oscilator STKBandedWG p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr BeeThree\n    a_oscilator STKBeeThree p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr BlowBotl\n    a_oscilator STKBlowBotl p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr BlowHole\n    a_oscilator STKBlowHole p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Bowed\n    a_oscilator STKBowed p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Brass\n    a_oscilator STKBrass p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Clarinet\n    a_oscilator STKClarinet p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Drummer\n    a_oscilator STKDrummer p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr FMVoices\n    a_oscilator STKFMVoices p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Flute\n    a_oscilator STKFlute p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr HevyMetl\n    a_oscilator STKHevyMetl p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Mandolin\n    a_oscilator STKMandolin p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr ModalBar\n    a_oscilator STKModalBar p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Moog\n    a_oscilator STKMoog p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr PercFlut\n    a_oscilator STKPercFlut p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Plucked\n    a_oscilator STKPlucked p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Resonate\n    a_oscilator STKResonate p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Rhodey\n    a_oscilator STKRhodey p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Saxofony\n    a_oscilator STKSaxofony p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Shakers\n    a_oscilator STKShakers p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Simple\n    a_oscilator STKSimple p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Sitar\n    a_oscilator STKSitar p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr StifKarp\n    a_oscilator STKStifKarp p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr TubeBell\n    a_oscilator STKTubeBell p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr VoicForm\n    a_oscilator STKVoicForm p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Whistle\n    a_oscilator STKWhistle p4, p5\n    outs a_oscilator, a_oscilator\nendin\ninstr Wurley\n    a_oscilator STKWurley p4, p5\n    outs a_oscilator, a_oscilator\nendin\n",
    "tempo": 200,
    "volume_percent": 50
}
    )"""";
    test_file.close();

  } else {
    cannot_open_error(test_file.fileName());
  }
  editor.load_from(test_file.fileName());
}

void Tester::test_column_headers() {
  auto &song = editor.song;
  // no symbol header
  QCOMPARE(get_column_heading(symbol_column),
           QVariant());
  QCOMPARE(get_column_heading(numerator_column),
           "Numerator");
  QCOMPARE(get_column_heading(denominator_column),
           "Denominator");
  QCOMPARE(get_column_heading(octave_column),
           "Octave");
  QCOMPARE(get_column_heading(beats_column),
           "Beats");
  QCOMPARE(
      get_column_heading(volume_percent_column),
      "Volume Percent");
  QCOMPARE(get_column_heading(tempo_percent_column),
           "Tempo Percent");
  QCOMPARE(get_column_heading(words_column),
           "Words");
  QCOMPARE(get_column_heading(instrument_column),
           "Instrument");
  // error for non-existent column
  QCOMPARE(get_column_heading(-1), QVariant());
  // no vertical labels
  QCOMPARE(song.headerData(numerator_column, Qt::Vertical, Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(
      song.headerData(numerator_column, Qt::Horizontal, Qt::DecorationRole),
      QVariant());
}

void Tester::test_song() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;

  // test saving
  song.save_to(test_file.fileName());

  auto &root = song.root;

  // misc errors
  error_row(-1);
  editor.song.root.assert_not_root();
  cannot_open_error("");
  assert_not_empty(QModelIndexList());
  editor.csound_data.start_song({"csound", "only 1 argument"});

  auto root_index = QModelIndex();
  QCOMPARE(song.rowCount(root_index), 2);
  QCOMPARE(song.columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(song.root.get_level(), ROOT_LEVEL);
}

void Tester::run_actions(QModelIndex &parent_index) {
  auto &undo_stack = editor.undo_stack;
  editor.copy(0, 1, parent_index);
  // paste after first chord
  editor.paste(0, parent_index);
  editor.undo_stack.undo();
  editor.insert(0, 1, parent_index);
  undo_stack.undo();
  editor.remove(0, 1, parent_index);
  undo_stack.undo();
  editor.play(0, 1, parent_index);
  // first cut off early
  editor.play(0, 1, parent_index);
  // now play the whole thing
  std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIME));
}

void Tester::test_actions() {
  auto root_index = QModelIndex();
  run_actions(root_index);
  auto first_chord_symbol_index =
      editor.song.index(0, symbol_column, root_index);
  run_actions(first_chord_symbol_index);
}

void Tester::test_chord() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index =
      song.index(0, symbol_column, root_index);
  auto first_chord_numerator_index =
      song.index(0, numerator_column, root_index);
  auto first_chord_instrument_index =
      song.index(0, instrument_column, root_index);
  auto &first_chord_node = song.root.get_child(0);
  auto *first_chord_pointer = first_chord_node.note_chord_pointer.get();

  // test first chord
  QCOMPARE(first_chord_node.get_level(), CHORD_LEVEL);
  QCOMPARE(song.parent(first_chord_symbol_index), root_index);
  QCOMPARE(first_chord_node.get_ratio(), 1.0);
  // only nest the symbol column
  QCOMPARE(song.rowCount(first_chord_numerator_index), 0);

  // cant edit the symbol
  QCOMPARE(song.flags(first_chord_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(song.flags(first_chord_numerator_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  // cant edit the instrument
  QCOMPARE(song.flags(first_chord_instrument_index), Qt::NoItemFlags);
  // error on non-existent column
  QCOMPARE(first_chord_pointer->flags(NON_EXISTENT_COLUMN), Qt::NoItemFlags);
}

void Tester::test_set_data() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto &undo_stack = editor.undo_stack;
  auto first_chord_symbol_index =
      song.index(0, symbol_column, root_index);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);
  
  set_data(0, symbol_column, root_index, QVariant());
  QVERIFY(set_data(0, numerator_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, denominator_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, octave_column, root_index, QVariant(1)));
  undo_stack.undo();
  QVERIFY(set_data(0, beats_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, tempo_percent_column, root_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, words_column, root_index, QVariant("hello")));
  undo_stack.undo();
  QVERIFY(!(set_data(0, instrument_column, root_index, QVariant())));

  // can't set non-existent column
  song.node_from_index(first_chord_symbol_index).note_chord_pointer->setData(NON_EXISTENT_COLUMN, QVariant());
  // setData only works for the edit role
  QVERIFY(!(song.setData(first_chord_symbol_index, QVariant(), Qt::DecorationRole)));

  QVERIFY(!(set_data(0, symbol_column, first_chord_symbol_index, QVariant())));
  QVERIFY(set_data(0, numerator_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, denominator_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, octave_column, first_chord_symbol_index, QVariant(1)));
  undo_stack.undo();
  QVERIFY(set_data(0, beats_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, volume_percent_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, tempo_percent_column, first_chord_symbol_index, QVariant(2)));
  undo_stack.undo();
  QVERIFY(set_data(0, words_column, first_chord_symbol_index, QVariant("hello")));
  undo_stack.undo();
  QVERIFY(!(set_data(0, instrument_column, first_chord_symbol_index, QVariant())));

  // can't set non-existent column
  song.node_from_index(first_note_symbol_index).note_chord_pointer ->setData(NON_EXISTENT_COLUMN, QVariant());
   // setData only works for the edit role
  QVERIFY(
      song.setData(first_note_symbol_index, QVariant(), Qt::DecorationRole));
  
}

void Tester::test_data() {

  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index =
      song.index(0, symbol_column, root_index);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);
  
  QCOMPARE(get_data(0, symbol_column, root_index, Qt::ForegroundRole),
           QVariant("♫"));
  QCOMPARE(get_data(0, numerator_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_NUMERATOR));
  QCOMPARE(get_data(0, denominator_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_DENOMINATOR));
  QCOMPARE(get_data(0, octave_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_OCTAVE));
  QCOMPARE(get_data(0, beats_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_VOLUME_PERCENT));
  QCOMPARE(get_data(0, tempo_percent_column, root_index, Qt::ForegroundRole),
           QVariant(DEFAULT_TEMPO_PERCENT));
  QCOMPARE(get_data(0, words_column, root_index, Qt::ForegroundRole),
           QVariant(""));
  QCOMPARE(get_data(0, instrument_column, root_index, Qt::ForegroundRole),
           QVariant());

  // error on non-existent column
  QCOMPARE(song.node_from_index(first_chord_symbol_index).note_chord_pointer->data(NON_EXISTENT_COLUMN, Qt::DisplayRole),
           QVariant());
  // empty for non-display data
  QCOMPARE(song.data(first_chord_symbol_index, Qt::DecorationRole), QVariant());
  
  QCOMPARE(get_data(0, symbol_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant("♫"));
  QCOMPARE(get_data(0, numerator_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_NUMERATOR));
  QCOMPARE(get_data(0, denominator_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_DENOMINATOR));
  QCOMPARE(get_data(0, octave_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_OCTAVE));
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_BEATS));
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_VOLUME_PERCENT));
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_TEMPO_PERCENT));
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(""));
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index, Qt::ForegroundRole),
           QVariant(DEFAULT_DEFAULT_INSTRUMENT));
  
  // error on non-existent column
  QCOMPARE(song.node_from_index(first_note_symbol_index).note_chord_pointer -> data(NON_EXISTENT_COLUMN, Qt::DisplayRole),
           QVariant());
  // empty for non display data
  QCOMPARE(song.data(first_note_symbol_index, Qt::DecorationRole), QVariant());
}

void Tester::test_colors() {
  auto &song = editor.song;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index =
      song.index(0, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(0);

  QCOMPARE(get_data(0, symbol_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(0, numerator_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, denominator_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, octave_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, beats_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, volume_percent_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, tempo_percent_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, words_column, root_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, instrument_column, root_index, Qt::ForegroundRole),
           NO_DATA);

  QCOMPARE(get_data(1, numerator_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, denominator_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, octave_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, beats_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, volume_percent_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, tempo_percent_column, root_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, words_column, root_index, Qt::ForegroundRole),
           NO_DATA);


  QCOMPARE(get_data(0, symbol_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(0, numerator_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, denominator_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, octave_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, beats_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, volume_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, tempo_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, words_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);
  QCOMPARE(get_data(0, instrument_column, first_chord_symbol_index, Qt::ForegroundRole),
           LIGHT_GRAY);

  QCOMPARE(get_data(1, numerator_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, denominator_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, octave_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, beats_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, volume_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, tempo_percent_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, words_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
  QCOMPARE(get_data(1, instrument_column, first_chord_symbol_index, Qt::ForegroundRole),
           NO_DATA);
}


void Tester::test_note() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  auto &root = song.root;
  auto root_index = QModelIndex();
  auto first_chord_symbol_index =
      song.index(0, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(0);

  // add some fields from the first note
  auto &first_note_node = first_chord_node.get_child(0);
  auto first_note_symbol_index =
      song.index(0, symbol_column, first_chord_symbol_index);
  auto first_note_numerator_index =
      song.index(0, numerator_column, first_chord_symbol_index);
  auto *first_note_pointer = first_note_node.note_chord_pointer.get();

  // test first note
  QCOMPARE(song.parent(first_note_symbol_index).row(), 0);
  QCOMPARE(first_note_node.get_level(), NOTE_LEVEL);

  // cant edit the symbol
  QCOMPARE(song.flags(first_note_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(song.flags(first_note_numerator_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  // error on non-existent column
  QCOMPARE(first_note_pointer->flags(NON_EXISTENT_COLUMN), Qt::NoItemFlags);

  // test some errors
  first_note_node.assert_child_at(-1);
  first_note_node.assert_insertable_at(-1);
  root.new_child_pointer(&first_note_node);
}
