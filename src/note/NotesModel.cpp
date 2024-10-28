#include "note/NotesModel.hpp"

#include <QList>
#include <QString>
#include <QtGlobal>

#include "chord/ChordsModel.hpp"
#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "note/Note.hpp"

// header functions

NotesModel::NotesModel(ChordsModel *parent_chords_model_pointer_input,
                       QObject *parent_pointer)
    : RowsModel<Note>(parent_chords_model_pointer_input->undo_stack_pointer,
                      nullptr, parent_pointer),
      parent_chords_model_pointer(parent_chords_model_pointer_input) {
  Q_ASSERT(parent_chords_model_pointer != nullptr);
}

auto NotesModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_NOTE_COLUMNS;
}

auto NotesModel::get_column_name(int column_number) const -> QString {
  switch (to_note_column(column_number)) {
  case note_instrument_column:
    return NotesModel::tr("Instrument");
  case note_interval_column:
    return NotesModel::tr("Interval");
  case note_beats_column:
    return NotesModel::tr("Beats");
  case note_velocity_ratio_column:
    return NotesModel::tr("Velocity ratio");
  case note_words_column:
    return NotesModel::tr("Words");
  }
}

auto NotesModel::get_status(int row_number) const -> QString {
  Q_ASSERT(rows_pointer != nullptr);
  return get_key_text(
      *parent_chords_model_pointer, parent_chord_number,
      interval_to_double(rows_pointer->at(row_number).interval));
};
