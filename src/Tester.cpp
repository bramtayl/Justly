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

#include "Chord.h"      // for CHORD_LEVEL
#include "Note.h"       // for NOTE_LEVEL
#include "NoteChord.h"  // for NoteChord, numerator_column, symbol_...
#include "Song.h"       // for Song, NOTE_CHORD_COLUMNS
#include "TreeNode.h"   // for TreeNode, ROOT_LEVEL
#include "Utilities.h"

const auto NON_EXISTENT_COLUMN = -1;

const auto TWO_DOUBLE = 2.0;

auto Tester::get_data(int row, int column, QModelIndex &parent_index)
    -> QVariant {
  auto &song = editor.song;
  return song.data(song.index(row, column, parent_index), Qt::DisplayRole);
}

auto Tester::set_data(int row, int column, QModelIndex &parent_index,
                      const QVariant &new_value) -> void {
  editor.setData(editor.song.index(row, column, parent_index), new_value,
                 Qt::EditRole);
}

void Tester::set_unset_field(int row, int column, QModelIndex &parent_index,
                            const QVariant &expected_value,
                            const QVariant &new_value) {
  auto original_value = get_data(row, column, parent_index);
  QCOMPARE(original_value, expected_value);
  set_data(row, column, parent_index, new_value);
  QCOMPARE(get_data(row, column, parent_index), new_value);
  editor.undo_stack.undo();
  QCOMPARE(get_data(row, column, parent_index), original_value);
}

void Tester::set_unset_picky_field(int row, int column,
                                  QModelIndex &parent_index,
                                  const QVariant &expected_value,
                                  const QVariant &invalid_value,
                                  const QVariant &valid_value) {
  auto original_value = get_data(row, column, parent_index);
  QCOMPARE(original_value, expected_value);
  set_data(row, column, parent_index, invalid_value);
  QCOMPARE(get_data(row, column, parent_index), original_value);
  set_data(row, column, parent_index, valid_value);
  QCOMPARE(get_data(row, column, parent_index), valid_value);
  editor.undo_stack.undo();
  QCOMPARE(get_data(row, column, parent_index), original_value);
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
                    "denominator": 4,
                    "numerator": 5
                },
                {
                    "denominator": 2,
                    "numerator": 3
                }
            ]
        },
        {
            "children": [
                {
                    "denominator": 2,
                    "numerator": 3
                },
                {
                    "octave": 1
                },
                {
                    "denominator": 4,
                    "numerator": 5,
                    "octave": 1
                }
            ],
            "denominator": 3,
            "numerator": 2
        },
        {
            "volume_ratio": 2.0,
            "tempo_ratio": 2.0,
            "words": "hello",
            "octave": 2,
            "children": [
                {
                    "beats": 2
                },
                {
                    "beats": 2,
                    "denominator": 4,
                    "numerator": 5
                },
                {
                    "beats": 2,
                    "denominator": 2,
                    "numerator": 3,
                    "volume_ratio": 2.0,
                    "tempo_ratio": 2.0,
                    "words": "hello",
                    "instrument": "Wurley"
                }
            ],
            "denominator": 2,
            "numerator": 3
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
  QCOMPARE(song.headerData(symbol_column, Qt::Horizontal, Qt::DisplayRole),
           QVariant());
  QCOMPARE(song.headerData(numerator_column, Qt::Horizontal, Qt::DisplayRole),
           "Numerator");
  QCOMPARE(song.headerData(denominator_column, Qt::Horizontal, Qt::DisplayRole),
           "Denominator");
  QCOMPARE(song.headerData(octave_column, Qt::Horizontal, Qt::DisplayRole),
           "Octave");
  QCOMPARE(song.headerData(beats_column, Qt::Horizontal, Qt::DisplayRole),
           "Beats");
  QCOMPARE(
      song.headerData(volume_ratio_column, Qt::Horizontal, Qt::DisplayRole),
      "Volume Ratio");
  QCOMPARE(song.headerData(tempo_ratio_column, Qt::Horizontal, Qt::DisplayRole),
           "Tempo Ratio");
  QCOMPARE(song.headerData(words_column, Qt::Horizontal, Qt::DisplayRole),
           "Words");
  QCOMPARE(song.headerData(instrument_column, Qt::Horizontal, Qt::DisplayRole),
           "Instrument");
  // error for non-existent column
  QCOMPARE(song.headerData(-1, Qt::Horizontal, Qt::DisplayRole), QVariant());
  // no vertical labels
  QCOMPARE(song.headerData(numerator_column, Qt::Vertical, Qt::DisplayRole),
           QVariant());
  // headers only for display role
  QCOMPARE(
      song.headerData(numerator_column, Qt::Horizontal, Qt::DecorationRole),
      QVariant());
}

void Tester::test_sliders() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  editor.set_frequency_with_slider();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_frequency_label(song.frequency);
  editor.set_volume_percent_with_silder();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_volume_percent_label(song.volume_percent);
  editor.set_tempo_with_slider();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_tempo_label(song.tempo);
}

void Tester::test_song() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;

  editor.save_orchestra_text();
  editor.set_default_instrument();
  // test saving
  song.save_to(test_file.fileName());

  auto &root = song.root;

  // misc errors
  error_row(-1);
  editor.song.root.assert_not_root();
  cannot_open_error("");
  assert_not_empty(QModelIndexList());

  auto root_index = QModelIndex();
  QCOMPARE(song.rowCount(root_index), 3);
  QCOMPARE(song.columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(song.root.get_level(), ROOT_LEVEL);
}

void Tester::run_actions(QModelIndex& parent_index) {
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
}

void Tester::test_actions() {
  auto root_index = QModelIndex();
  run_actions(root_index);
  auto first_chord_symbol_index = editor.song.index(0, symbol_column, root_index);
  run_actions(first_chord_symbol_index);
}

void Tester::test_chord() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  auto root_index = QModelIndex();
  auto first_chord_index = 0;
  auto first_chord_symbol_index =
      song.index(first_chord_index, symbol_column, root_index);
  auto first_chord_numerator_index =
      song.index(first_chord_index, numerator_column, root_index);
  auto first_chord_instrument_index =
      song.index(first_chord_index, instrument_column, root_index);
  auto &first_chord_node = song.root.get_child(first_chord_index);
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

  QCOMPARE(song.data(first_chord_symbol_index, Qt::DisplayRole), QVariant("♫"));
  // no instrument data
  QCOMPARE(song.data(first_chord_instrument_index, Qt::DisplayRole),
           QVariant());
  // error on non-existent column
  QCOMPARE(first_chord_pointer->data(NON_EXISTENT_COLUMN, Qt::DisplayRole),
           QVariant());
  // empty for non-display data
  QCOMPARE(song.data(first_chord_symbol_index, Qt::DecorationRole), QVariant());
  // setData only works for the edit role
  QVERIFY(
      song.setData(song.index(first_chord_index, numerator_column, root_index),
                   QVariant(), Qt::DecorationRole));
  // can't set non-existent column
  QVERIFY(!(first_chord_pointer->setData(NON_EXISTENT_COLUMN, QVariant(),
                                         Qt::EditRole)));

  set_unset_picky_field(first_chord_index, numerator_column, root_index,
                       QVariant(DEFAULT_NUMERATOR), QVariant(-1), QVariant(2));
  set_unset_picky_field(first_chord_index, denominator_column, root_index,
                       QVariant(DEFAULT_DENOMINATOR), QVariant(-1),
                       QVariant(2));
  set_unset_field(first_chord_index, octave_column, root_index,
                 QVariant(DEFAULT_OCTAVE), QVariant(1));
  set_unset_field(first_chord_index, beats_column, root_index,
                 QVariant(DEFAULT_BEATS), QVariant(2));
  set_unset_picky_field(first_chord_index, volume_ratio_column, root_index,
                       QVariant(DEFAULT_VOLUME_RATIO), QVariant(-1.0),
                       QVariant(TWO_DOUBLE));
  set_unset_picky_field(first_chord_index, tempo_ratio_column, root_index,
                       QVariant(DEFAULT_TEMPO_RATIO), QVariant(-1.0),
                       QVariant(TWO_DOUBLE));
  set_unset_field(first_chord_index, words_column, root_index, QVariant(""),
                 QVariant("hello"));
  
}

void Tester::test_note() {
  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;
  auto &root = song.root;
  auto root_index = QModelIndex();
  auto first_chord_index = 0;
  auto first_chord_symbol_index =
      song.index(first_chord_index, symbol_column, root_index);
  auto &first_chord_node = song.root.get_child(first_chord_index);

  // add some fields from the first note
  auto first_note_index = 0;
  auto &first_note_node = first_chord_node.get_child(first_note_index);
  auto first_note_symbol_index =
      song.index(first_note_index, symbol_column, first_chord_symbol_index);
  auto first_note_numerator_index =
      song.index(first_note_index, numerator_column, first_chord_symbol_index);
  auto *first_note_pointer = first_note_node.note_chord_pointer.get();

  // test first note
  QCOMPARE(song.parent(first_note_symbol_index).row(), first_chord_index);
  QCOMPARE(first_note_node.get_level(), NOTE_LEVEL);

  // cant edit the symbol
  QCOMPARE(song.flags(first_note_symbol_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  QCOMPARE(song.flags(first_note_numerator_index),
           Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
  // error on non-existent column
  QCOMPARE(first_note_pointer->flags(NON_EXISTENT_COLUMN), Qt::NoItemFlags);

  QCOMPARE(song.data(first_note_symbol_index, Qt::DisplayRole), QVariant("♪"));
  // error on non-existent column
  QCOMPARE(first_note_pointer->data(NON_EXISTENT_COLUMN, Qt::DisplayRole),
           QVariant());
  // empty for non display data
  QCOMPARE(song.data(first_note_symbol_index, Qt::DecorationRole), QVariant());

  // setData only works for the edit role
  QVERIFY(
      song.setData(first_note_numerator_index, QVariant(), Qt::DecorationRole));
  // can't set non-existent column
  QVERIFY(!(first_note_pointer->setData(NON_EXISTENT_COLUMN, QVariant(),
                                        Qt::EditRole)));

  set_unset_picky_field(first_note_index, numerator_column,
                       first_chord_symbol_index, QVariant(DEFAULT_NUMERATOR),
                       QVariant(-1), QVariant(2));
  set_unset_picky_field(first_note_index, denominator_column,
                       first_chord_symbol_index, QVariant(DEFAULT_DENOMINATOR),
                       QVariant(-1), QVariant(2));
  set_unset_field(first_note_index, octave_column, first_chord_symbol_index,
                 QVariant(DEFAULT_OCTAVE), QVariant(1));
  set_unset_field(first_note_index, beats_column, first_chord_symbol_index,
                 QVariant(DEFAULT_BEATS), QVariant(2));
  set_unset_picky_field(first_note_index, volume_ratio_column,
                       first_chord_symbol_index, QVariant(DEFAULT_VOLUME_RATIO),
                       QVariant(-1.0), QVariant(TWO_DOUBLE));
  set_unset_picky_field(first_note_index, tempo_ratio_column,
                       first_chord_symbol_index, QVariant(DEFAULT_TEMPO_RATIO),
                       QVariant(-1.0), QVariant(TWO_DOUBLE));
  set_unset_field(first_note_index, words_column, first_chord_symbol_index,
                 QVariant(""), QVariant("hello"));
  set_unset_picky_field(first_note_index, instrument_column,
                       first_chord_symbol_index, QVariant("Plucked"),
                       QVariant("not an instrument"), QVariant("Wurley"));

  // test some errors
  first_note_node.assert_child_at(-1);
  first_note_node.assert_insertable_at(-1);
  root.new_child_pointer(&first_note_node);

}
