#include "pitched_note/PitchedNotesModel.hpp"

#include <QList>
#include <QString>
#include <QtGlobal>

#include "interval/Interval.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "pitched_note/PitchedNote.hpp"
#include "song/Song.hpp"

class QUndoStack;

PitchedNotesModel::PitchedNotesModel(
    QUndoStack& undo_stack, Song& song_input, QObject *parent_pointer)
    : RowsModel<PitchedNote>(
          undo_stack, nullptr,
          parent_pointer),
      song(song_input) {
}

auto PitchedNotesModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_PITCHED_NOTE_COLUMNS;
}

auto PitchedNotesModel::get_column_name(int column_number) const -> QString {
  switch (to_pitched_note_column(column_number)) {
  case pitched_note_instrument_column:
    return PitchedNotesModel::tr("Instrument");
  case pitched_note_interval_column:
    return PitchedNotesModel::tr("Interval");
  case pitched_note_beats_column:
    return PitchedNotesModel::tr("Beats");
  case pitched_note_velocity_ratio_column:
    return PitchedNotesModel::tr("Velocity ratio");
  case pitched_note_words_column:
    return PitchedNotesModel::tr("Words");
  }
}

auto PitchedNotesModel::get_status(int row_number) const -> QString {
  Q_ASSERT(rows_pointer != nullptr);
  return get_key_text(
      song, parent_chord_number,
      interval_to_double(rows_pointer->at(row_number).interval));
};
