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

#include <memory>  // for unique_ptr

#include "Chord.h"      // for CHORD_LEVEL
#include "Note.h"       // for NOTE_LEVEL
#include "NoteChord.h"  // for NoteChord, numerator_column, symbol_...
#include "Song.h"       // for Song, NOTE_CHORD_COLUMNS
#include "TreeNode.h"   // for TreeNode, ROOT_LEVEL

const auto TEST_INT = 2;
const auto TEST_NEGATIVE_INT = -1;
const auto TEST_DOUBLE = 2.0;
const auto TEST_NEGATIVE_DOUBLE = -2.0;
const auto TEST_FRACTION = 0.5;
const auto NON_EXISTENT_COLUMN = -1;

Tester::Tester(const QString &examples_folder_input)
    : examples_folder(examples_folder_input) {}

void Tester::test_positive_int_field(int row, int column,
                                     QModelIndex &parent_index) {
  auto &song = editor.song;
  auto previous_value =
      song.data(song.index(row, column, parent_index), Qt::DisplayRole);
  editor.setData(song.index(row, column, parent_index),
                 QVariant(TEST_NEGATIVE_INT), Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           previous_value);
  editor.setData(song.index(row, column, parent_index), QVariant(TEST_INT),
                 Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           TEST_INT);
  editor.undo_stack.undo();
}

void Tester::test_int_field(int row, int column, QModelIndex &parent_index) {
  auto &song = editor.song;
  auto previous_value =
      song.data(song.index(row, column, parent_index), Qt::DisplayRole);
  editor.setData(song.index(row, column, parent_index),
                 QVariant(TEST_NEGATIVE_INT), Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           QVariant(TEST_NEGATIVE_INT));
  editor.undo_stack.undo();
}

void Tester::test_positive_double_field(int row, int column,
                                        QModelIndex &parent_index) {
  auto &song = editor.song;
  auto previous_value =
      song.data(song.index(row, column, parent_index), Qt::DisplayRole);
  editor.setData(song.index(row, column, parent_index),
                 QVariant(TEST_NEGATIVE_DOUBLE), Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           previous_value);
  editor.setData(song.index(row, column, parent_index), QVariant(TEST_DOUBLE),
                 Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           TEST_DOUBLE);
  editor.undo_stack.undo();
}

void Tester::test_string_field(int row, int column, QModelIndex &parent_index) {
  auto &song = editor.song;
  auto previous_value =
      song.data(song.index(row, column, parent_index), Qt::DisplayRole);
  editor.setData(song.index(row, column, parent_index), QVariant("hello"),
                 Qt::EditRole);
  QCOMPARE(song.data(song.index(row, column, parent_index), Qt::DisplayRole),
           QVariant("hello"));
  editor.undo_stack.undo();
}

void Tester::test_everything() {
  // test json parsing
  QJsonObject dummy_object;

  NoteChord::get_string(dummy_object, "name", "");
  dummy_object["name"] = "a";
  NoteChord::get_double(dummy_object, "name", 1.0);
  dummy_object["name"] = -1;
  NoteChord::get_positive_double(dummy_object, "name", 1.0);
  dummy_object["name"] = TEST_FRACTION;
  NoteChord::get_int(dummy_object, "name", 1);
  dummy_object["name"] = 0;
  NoteChord::get_positive_int(dummy_object, "name", 1);
  dummy_object["name"] = -1;
  NoteChord::get_non_negative_int(dummy_object, "name", 1);

  auto &song = editor.song;
  auto &undo_stack = editor.undo_stack;

  auto simple_path = examples_folder.filePath("simple.json");

  editor.load(simple_path);
  // test saving
  song.save(simple_path);
  // should error
  editor.load(examples_folder.filePath("malformed.json"));
  editor.load(examples_folder.filePath("not_song.json"));

  auto &root = song.root;

  // test some errors
  TreeNode::error_row(-1);
  TreeNode::error_not_object();
  root.assert_not_root();

  // test song
  auto root_index = QModelIndex();
  QCOMPARE(song.rowCount(root_index), 3);
  QCOMPARE(song.columnCount(), NOTE_CHORD_COLUMNS);
  QCOMPARE(song.root.get_level(), ROOT_LEVEL);

  // test song actions
  editor.set_frequency();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_frequency_label(song.frequency);
  editor.set_volume_percent();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_volume_percent_label(song.volume_percent);
  editor.set_tempo();
  undo_stack.undo();
  undo_stack.redo();
  undo_stack.undo();
  editor.set_tempo_label(song.tempo);

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
  // only display headers
  QCOMPARE(
      song.headerData(numerator_column, Qt::Horizontal, Qt::DecorationRole),
      QVariant());

  // test loading broken chord
  QJsonObject broken_object;
  broken_object["children"] = 0;
  // should error
  root.load_children(broken_object);

  // test loading broken chords
  QJsonArray broken_array;
  broken_array.append(1);
  broken_object["children"] = broken_array;
  // should error
  root.load_children(broken_object);

  // add some fields from the first chord
  auto first_chord_number = 0;
  auto first_chord_symbol_index =
      song.index(first_chord_number, symbol_column, root_index);
  auto first_chord_numerator_index =
      song.index(first_chord_number, numerator_column, root_index);
  auto first_chord_instrument_index =
      song.index(first_chord_number, instrument_column, root_index);
  auto &first_chord_node = song.root.get_child(first_chord_number);
  auto *first_chord_pointer = first_chord_node.note_chord_pointer.get();

  // test first chord
  QCOMPARE(first_chord_node.get_level(), CHORD_LEVEL);
  QCOMPARE(song.parent(first_chord_symbol_index), root_index);
  QCOMPARE(first_chord_node.get_ratio(), 1.0F);
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

  test_positive_int_field(first_chord_number, numerator_column, root_index);
  test_positive_int_field(first_chord_number, denominator_column, root_index);
  test_int_field(first_chord_number, octave_column, root_index);
  test_int_field(first_chord_number, beats_column, root_index);
  test_positive_double_field(first_chord_number, volume_ratio_column,
                             root_index);
  test_positive_double_field(first_chord_number, tempo_ratio_column,
                             root_index);
  test_string_field(first_chord_number, words_column, root_index);

  QVERIFY(!(first_chord_pointer->setData(NON_EXISTENT_COLUMN, QVariant(),
                                         Qt::EditRole)));
  // non-edit doesnt apply
  QVERIFY(
      song.setData(song.index(first_chord_number, numerator_column, root_index),
                   QVariant(), Qt::DecorationRole));

  // test chord actions
  editor.play(first_chord_symbol_index, 3);
  editor.copy(first_chord_symbol_index, 3);
  editor.paste(1, root_index);
  undo_stack.undo();
  editor.insert(1, 1, root_index);
  undo_stack.undo();
  editor.remove(1, 2, root_index);
  undo_stack.undo();
  editor.remove(1, 2, first_chord_symbol_index);
  undo_stack.undo();
  editor.remove(0, 1, first_chord_symbol_index);
  undo_stack.undo();

  // add some fields from the first note
  auto first_note_number = 0;
  auto &first_note_node = first_chord_node.get_child(first_note_number);
  auto first_note_symbol_index =
      song.index(first_note_number, symbol_column, first_chord_symbol_index);
  auto first_note_numerator_index =
      song.index(first_note_number, numerator_column, first_chord_symbol_index);
  auto *first_note_pointer = first_note_node.note_chord_pointer.get();

  // test first note
  QCOMPARE(song.parent(first_note_symbol_index).row(), first_chord_number);
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

  // non-edit doesnt apply
  QVERIFY(
      song.setData(first_note_numerator_index, QVariant(), Qt::DecorationRole));
  // error on non-existent column
  QVERIFY(!(first_note_pointer->setData(NON_EXISTENT_COLUMN, QVariant(),
                                        Qt::EditRole)));

  test_positive_int_field(first_note_number, numerator_column,
                          first_chord_symbol_index);
  test_positive_int_field(first_note_number, denominator_column,
                          first_chord_symbol_index);
  test_int_field(first_note_number, octave_column, first_chord_symbol_index);
  test_positive_int_field(first_note_number, beats_column,
                          first_chord_symbol_index);
  test_positive_double_field(first_note_number, volume_ratio_column,
                             first_chord_symbol_index);
  test_positive_double_field(first_note_number, tempo_ratio_column,
                             first_chord_symbol_index);
  test_string_field(first_note_number, words_column, first_chord_symbol_index);
  test_string_field(first_note_number, instrument_column,
                    first_chord_symbol_index);

  // test some errors
  first_note_node.assert_child_at(-1);
  first_note_node.assert_insertable_at(-1);
  TreeNode::new_child_note_chord_pointer(&first_note_node);

  // test note actions
  editor.play(first_note_symbol_index, 3);
  editor.copy(first_note_symbol_index, 3);
  editor.paste(1, first_chord_symbol_index);
  undo_stack.undo();
  editor.insert(1, 1, first_chord_symbol_index);
  undo_stack.undo();
  editor.setData(first_note_numerator_index, QVariant(2), Qt::EditRole);
  undo_stack.undo();

  editor.load(examples_folder.filePath("test.json"));
  // create a new index for the new song
  // the first note in the chord is very short
  // the second note uses a non-existant instrument
  editor.play(song.index(first_chord_number, symbol_column, root_index), 1);
}
