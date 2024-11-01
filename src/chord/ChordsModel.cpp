#include "chord/ChordsModel.hpp"

#include <QString>

#include "justly/ChordColumn.hpp"
#include "song/Song.hpp"

enum Degree {
  c_degree = 0,
  c_sharp_degree = 1,
  d_degree = 2,
  e_flat_degree = 3,
  e_degree = 4,
  f_degree = 5,
  f_sharp_degree = 6,
  g_degree = 7,
  a_flat_degree = 8,
  a_degree = 9,
  b_flat_degree = 10,
  b_degree = 11
};

ChordsModel::ChordsModel(QUndoStack &undo_stack, Song &song_input)
    : RowsModel(undo_stack), song(song_input) {
  rows_pointer = &song.chords;
}

auto ChordsModel::is_column_editable(int column_number) const -> bool {
  return column_number != chord_pitched_notes_column &&
         column_number != chord_unpitched_notes_column;
}

auto ChordsModel::get_status(int row_number) const -> QString {
  return get_key_text(song, row_number);
};
