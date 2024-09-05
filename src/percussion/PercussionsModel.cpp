#include "percussion/PercussionsModel.hpp"

#include <QAbstractItemModel>
#include <QBrush>
#include <QList>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QTextStream>
#include <QUndoStack>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <vector>

#include "chord/ChordsModel.hpp"
#include "justly/PercussionColumn.hpp"
#include "note/Note.hpp"
#include "other/templates.hpp"
#include "percussion/InsertPercussion.hpp"
#include "percussion/InsertPercussions.hpp"
#include "percussion/RemovePercussions.hpp"
#include "percussion/SetPercussionBeats.hpp"
#include "percussion/SetPercussionInstrument.hpp"
#include "percussion/SetPercussionSet.hpp"
#include "percussion/SetPercussionTempoRatio.hpp"
#include "percussion/SetPercussionVelocityRatio.hpp"
#include "percussion/SetPercussionsCells.hpp"
#include "percussion_instrument/PercussionInstrument.hpp"
#include "rational/Rational.hpp"

// IWYU pragma: no_include <algorithm>

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

PercussionsModel::PercussionsModel(std::vector<Percussion> &percussions_input,
                                   QUndoStack *undo_stack_pointer_input,
                                   QWidget *parent_pointer_input)
    : QAbstractTableModel(parent_pointer_input),
      parent_pointer(parent_pointer_input), percussions(percussions_input),
      undo_stack_pointer(undo_stack_pointer_input) {
  Q_ASSERT(undo_stack_pointer_input != nullptr);
}

auto PercussionsModel::rowCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return static_cast<int>(percussions.size());
}

auto PercussionsModel::columnCount(const QModelIndex & /*parent_index*/) const
    -> int {
  return NUMBER_OF_PERCUSSION_COLUMNS;
}

auto PercussionsModel::headerData(int column, Qt::Orientation orientation,
                                  int role) const -> QVariant {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    switch (to_percussion_column(column)) {
    case percussion_set_column:
      return PercussionsModel::tr("Percussion Set");
    case percussion_instrument_column:
      return PercussionsModel::tr("Percussion Instrument");
    case percussion_beats_column:
      return PercussionsModel::tr("Beats");
    case percussion_velocity_ratio_column:
      return PercussionsModel::tr("Velocity ratio");
    case percussion_tempo_ratio_column:
      return PercussionsModel::tr("Tempo ratio");
    }
  }
  // no horizontal headers
  // no headers for other roles
  return {};
}

auto PercussionsModel::flags(const QModelIndex & /*index*/) const
    -> Qt::ItemFlags {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
  ;
}

auto PercussionsModel::data(const QModelIndex &index,
                            int role) const -> QVariant {
  auto child_number = get_child_number(index);
  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    const auto &percussion = get_const_item(percussions, child_number);
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
    default:
      Q_ASSERT(false);
      return {};
    }
  }
  // no data for other roles
  return {};
}

auto PercussionsModel::setData(const QModelIndex &index,
                               const QVariant &new_value, int role) -> bool {
  // only set data for edit
  if (role != Qt::EditRole) {
    return false;
  }
  auto percussion_column = get_percussion_column(index);
  auto percussion_number = get_child_number(index);
  const auto &percussion = get_const_item(percussions, percussion_number);
  switch (percussion_column) {
  case percussion_set_column:
    Q_ASSERT(new_value.canConvert<const PercussionSet *>());
    undo_stack_pointer->push(std::make_unique<SetPercussionSet>(
                                 this, percussion_number,
                                 percussion.percussion_set_pointer,
                                 new_value.value<const PercussionSet *>())
                                 .release());
    break;
  case percussion_instrument_column:
    Q_ASSERT(new_value.canConvert<const PercussionInstrument *>());
    undo_stack_pointer->push(
        std::make_unique<SetPercussionInstrument>(
            this, percussion_number, percussion.percussion_instrument_pointer,
            new_value.value<const PercussionInstrument *>())
            .release());
    break;
  case percussion_beats_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetPercussionBeats>(
                                 this, percussion_number, percussion.beats,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case percussion_velocity_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(std::make_unique<SetPercussionVelocityRatio>(
                                 this, percussion_number,
                                 percussion.velocity_ratio,
                                 new_value.value<Rational>())
                                 .release());
    break;
  case percussion_tempo_ratio_column:
    Q_ASSERT(new_value.canConvert<Rational>());
    undo_stack_pointer->push(
        std::make_unique<SetPercussionTempoRatio>(this, percussion_number,
                                                  percussion.tempo_ratio,
                                                  new_value.value<Rational>())
            .release());
    break;
  }
  parent_pointer->setFocus();
  return true;
}

void PercussionsModel::edited_percussions_cells(size_t first_percussion_number,
                                                size_t number_of_percussions,
                                                PercussionColumn left_column,
                                                PercussionColumn right_column) {
  emit dataChanged(
      index(first_percussion_number, left_column),
      index(first_percussion_number + number_of_percussions - 1, right_column),
      {Qt::DisplayRole, Qt::EditRole});
}

void PercussionsModel::begin_insert_rows(size_t first_percussion_number,
                                         size_t number_of_percussions) {
  beginInsertRows(
      QModelIndex(), static_cast<int>(first_percussion_number),
      static_cast<int>(first_percussion_number + number_of_percussions) - 1);
}

void PercussionsModel::end_insert_rows() { endInsertRows(); }

void PercussionsModel::begin_remove_rows(size_t first_percussion_number,
                                         size_t number_of_percussions) {
  beginRemoveRows(
      QModelIndex(), static_cast<int>(first_percussion_number),
      static_cast<int>(first_percussion_number + number_of_percussions) - 1);
}

void PercussionsModel::end_remove_rows() { endRemoveRows(); }

void insert_percussion(PercussionsModel *percussions_model_pointer,
                       size_t percussion_number,
                       const Percussion &new_percussion) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  auto &percussions = percussions_model_pointer->percussions;

  check_end_number(percussions, percussion_number);

  percussions_model_pointer->begin_insert_rows(percussion_number, 1);
  percussions.insert(percussions.begin() + static_cast<int>(percussion_number),
                     new_percussion);
  percussions_model_pointer->end_insert_rows();
}

void insert_percussions(PercussionsModel *percussions_model_pointer,
                        size_t first_percussion_number,
                        const std::vector<Percussion> &new_percussions) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  auto &percussions = percussions_model_pointer->percussions;

  check_end_number(percussions, first_percussion_number);

  percussions_model_pointer->begin_insert_rows(first_percussion_number,
                                               new_percussions.size());
  percussions.insert(percussions.begin() +
                         static_cast<int>(first_percussion_number),
                     new_percussions.begin(), new_percussions.end());
  percussions_model_pointer->end_insert_rows();
}

void remove_percussions(PercussionsModel *percussions_model_pointer,
                        size_t first_percussion_number,
                        size_t number_of_percussions) {
  Q_ASSERT(percussions_model_pointer != nullptr);
  auto &percussions = percussions_model_pointer->percussions;
  check_range(percussions, first_percussion_number, number_of_percussions);

  percussions_model_pointer->begin_remove_rows(first_percussion_number,
                                               number_of_percussions);
  percussions.erase(
      percussions.begin() + static_cast<int>(first_percussion_number),
      percussions.begin() +
          static_cast<int>(first_percussion_number + number_of_percussions));
  percussions_model_pointer->end_remove_rows();
}
