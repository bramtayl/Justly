#include "chord/ChordsModel.hpp"

#include <QString>
#include <QtGlobal>

#include "chord/Chord.hpp"
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

// header functions

ChordsModel::ChordsModel(QUndoStack *undo_stack_pointer_input,
                         Song& song_input,
                         QObject *parent_pointer)
    : RowsModel(undo_stack_pointer_input, &song_input.chords, parent_pointer), song(song_input) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto ChordsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_CHORD_COLUMNS;
}

auto ChordsModel::get_column_name(int column_number) const -> QString {
  switch (to_chord_column(column_number)) {
  case chord_instrument_column:
    return ChordsModel::tr("Instrument");
  case chord_percussion_set_column:
    return ChordsModel::tr("Percussion set");
  case chord_percussion_instrument_column:
    return ChordsModel::tr("Percussion instrument");
  case chord_interval_column:
    return ChordsModel::tr("Interval");
  case chord_beats_column:
    return ChordsModel::tr("Beats");
  case chord_velocity_ratio_column:
    return ChordsModel::tr("Velocity ratio");
  case chord_tempo_ratio_column:
    return ChordsModel::tr("Tempo ratio");
  case chord_words_column:
    return ChordsModel::tr("Words");
  case chord_pitched_notes_column:
    return ChordsModel::tr("Notes");
  case chord_unpitched_notes_column:
    return ChordsModel::tr("Percussions");
  }
}

auto ChordsModel::is_column_editable(int column_number) const -> bool {
  return column_number != chord_pitched_notes_column &&
         column_number != chord_unpitched_notes_column;
}

auto ChordsModel::get_status(int row_number) const -> QString {
  return get_key_text(song, row_number);
};
