#include "percussion/PercussionsModel.hpp"

#include <QtGlobal>

#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"

class QObject;

PercussionsModel::PercussionsModel(QUndoStack *undo_stack_pointer_input,
                                   QObject *parent_pointer)
    : RowsModel<Percussion>(undo_stack_pointer_input, nullptr, parent_pointer) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto PercussionsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_PERCUSSION_COLUMNS;
}

auto PercussionsModel::get_column_name(int column_number) const -> QString {
  switch (to_percussion_column(column_number)) {
  case percussion_percussion_set_column:
    return PercussionsModel::tr("Percussion set");
  case percussion_percussion_instrument_column:
    return PercussionsModel::tr("Percussion instrument");
  case percussion_beats_column:
    return PercussionsModel::tr("Beats");
  case percussion_velocity_ratio_column:
    return PercussionsModel::tr("Velocity ratio");
  }
}

auto PercussionsModel::get_status(int /*row_number*/) const -> QString {
  return "";
};
