#include "unpitched_note/UnpitchedNotesModel.hpp"

#include <QtGlobal>

#include "justly/UnpitchedNoteColumn.hpp"
#include "unpitched_note/UnpitchedNote.hpp"

class QObject;

UnpitchedNotesModel::UnpitchedNotesModel(QUndoStack& undo_stack,
                                         QObject *parent_pointer)
    : RowsModel<UnpitchedNote>(undo_stack, nullptr,
                               parent_pointer) {
}

auto UnpitchedNotesModel::columnCount(
    const QModelIndex & /*parent_index*/) const -> int {
  return NUMBER_OF_UNPITCHED_NOTE_COLUMNS;
}

auto UnpitchedNotesModel::get_column_name(int column_number) const -> QString {
  switch (to_unpitched_note_column(column_number)) {
  case unpitched_note_percussion_set_column:
    return UnpitchedNotesModel::tr("Percussion set");
  case unpitched_note_percussion_instrument_column:
    return UnpitchedNotesModel::tr("Percussion instrument");
  case unpitched_note_beats_column:
    return UnpitchedNotesModel::tr("Beats");
  case unpitched_note_velocity_ratio_column:
    return UnpitchedNotesModel::tr("Velocity ratio");
  case unpitched_note_words_column:
    return UnpitchedNotesModel::tr("Words");
  }
}

auto UnpitchedNotesModel::get_status(int /*row_number*/) const -> QString {
  return "";
};
