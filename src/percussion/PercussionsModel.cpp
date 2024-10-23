#include "percussion/PercussionsModel.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QUndoStack>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "items_model/SetBeats.hpp"
#include "items_model/SetPercussionInstrument.hpp"
#include "items_model/SetPercussionSet.hpp"
#include "items_model/SetTempoRatio.hpp"
#include "items_model/SetVelocityRatio.hpp"
#include "justly/PercussionColumn.hpp"
#include "percussion/Percussion.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

class QObject;

static const auto NUMBER_OF_PERCUSSION_COLUMNS = 5;

[[nodiscard]] static auto
get_percussion_column(const QModelIndex &index) -> PercussionColumn {
  return to_percussion_column(index.column());
}

auto to_percussion_column(int column) -> PercussionColumn {
  Q_ASSERT(column >= 0);
  Q_ASSERT(column < NUMBER_OF_PERCUSSION_COLUMNS);
  return static_cast<PercussionColumn>(column);
}

PercussionsModel::PercussionsModel(QUndoStack *undo_stack_pointer_input,
                                   QObject *parent_pointer)
    : ItemsModel<Percussion>(nullptr, parent_pointer),
      undo_stack_pointer(undo_stack_pointer_input) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto PercussionsModel::get_percussion_set_column() const -> int {
  return percussion_set_column;
};

auto PercussionsModel::get_percussion_instrument_column() const -> int {
  return percussion_instrument_column;
};

auto PercussionsModel::get_beats_column() const -> int {
  return percussion_beats_column;
};

auto PercussionsModel::get_tempo_ratio_column() const -> int {
  return percussion_tempo_ratio_column;
};

auto PercussionsModel::get_velocity_ratio_column() const -> int {
  return percussion_velocity_ratio_column;
};

auto PercussionsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_PERCUSSION_COLUMNS;
}

auto PercussionsModel::get_column_name(int column_number) const -> QString {
  switch (to_percussion_column(column_number)) {
  case percussion_set_column:
    return PercussionsModel::tr("Percussion set");
  case percussion_instrument_column:
    return PercussionsModel::tr("Percussion instrument");
  case percussion_beats_column:
    return PercussionsModel::tr("Beats");
  case percussion_velocity_ratio_column:
    return PercussionsModel::tr("Velocity ratio");
  case percussion_tempo_ratio_column:
    return PercussionsModel::tr("Tempo ratio");
  }
}

auto PercussionsModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return {};
  }
  Q_ASSERT(items_pointer != nullptr);
  const auto &percussion = items_pointer->at(index.row());
  switch (get_percussion_column(index)) {
  case percussion_set_column:
    return QVariant::fromValue(percussion.percussion_set_pointer);
  case percussion_instrument_column:
    return QVariant::fromValue(percussion.percussion_instrument_pointer);
  case (percussion_beats_column):
    return QVariant::fromValue(percussion.beats);
  case percussion_velocity_ratio_column:
    return QVariant::fromValue(percussion.velocity_ratio);
  case percussion_tempo_ratio_column:
    return QVariant::fromValue(percussion.tempo_ratio);
  }
}

auto PercussionsModel::setData(const QModelIndex &index,
                               const QVariant &new_value, int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto percussion_column = get_percussion_column(index);
  auto percussion_number = index.row();
  Q_ASSERT(items_pointer != nullptr);
  const auto &percussion = items_pointer->at(percussion_number);
  switch (percussion_column) {
  case percussion_set_column:
    Q_ASSERT(new_value.canConvert<const PercussionSet *>());
    undo_stack_pointer->push(std::make_unique<SetPercussionSet<Percussion>>(
                                 this, percussion_number,
                                 percussion.percussion_set_pointer,
                                 new_value.value<const PercussionSet *>())
                                 .release());
    break;
  case percussion_instrument_column:
    Q_ASSERT(new_value.canConvert<const PercussionInstrument *>());
    undo_stack_pointer->push(
        std::make_unique<SetPercussionInstrument<Percussion>>(
            this, percussion_number, percussion.percussion_instrument_pointer,
            new_value.value<const PercussionInstrument *>())
            .release());
    break;
  case percussion_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetBeats<Percussion>>(
                                 this, percussion_number, percussion.beats,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case percussion_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetVelocityRatio<Percussion>>(
                                 this, percussion_number,
                                 percussion.velocity_ratio,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case percussion_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetTempoRatio<Percussion>>(this, percussion_number,
                                                    percussion.tempo_ratio,
                                                    new_value.value<Rational>())
            .release());
    break;
  }
  return true;
}
