#include "row/chord/ChordsModel.hpp"

#include <QString>

#include "song/Song.hpp"

ChordsModel::ChordsModel(QUndoStack &undo_stack, Song &song_input)
    : RowsModel(undo_stack), song(song_input) {
  rows_pointer = &song.chords;
}

auto ChordsModel::get_status(int row_number) const -> QString {
  return get_key_text(song, row_number);
};
