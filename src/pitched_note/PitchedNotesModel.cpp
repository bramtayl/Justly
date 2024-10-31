#include "pitched_note/PitchedNotesModel.hpp"

#include <QList>
#include <QString>
#include <QtGlobal>

#include "interval/Interval.hpp"
#include "pitched_note/PitchedNote.hpp"
#include "song/Song.hpp"

class QUndoStack;

PitchedNotesModel::PitchedNotesModel(QUndoStack &undo_stack, Song &song_input)
    : RowsModel<PitchedNote>(undo_stack),
      song(song_input) {}

auto PitchedNotesModel::get_status(int row_number) const -> QString {
  return get_key_text(
      song, parent_chord_number,
      interval_to_double(get_const_rows(*this).at(row_number).interval));
};
