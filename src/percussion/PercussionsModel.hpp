#pragma once

#include <QString>
#include <QVariant>

#include "justly/PercussionColumn.hpp"
#include "items_model/ItemsModel.hpp"

struct Percussion;
class QObject;
class QModelIndex;
class QUndoStack;
template <typename T> class QList;

[[nodiscard]] auto to_percussion_column(int column) -> PercussionColumn;

struct PercussionsModel : public ItemsModel<Percussion> {
  QList<Percussion> *items_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit PercussionsModel(QUndoStack *undo_stack_pointer_input,
                            QObject *parent_pointer = nullptr);
  // override functions
  [[nodiscard]] auto get_percussion_set_column() const -> int override;
  [[nodiscard]] auto get_percussion_instrument_column() const -> int override;
  [[nodiscard]] auto get_beats_column() const -> int override;
  [[nodiscard]] auto get_tempo_ratio_column() const -> int override;
  [[nodiscard]] auto get_velocity_ratio_column() const -> int override;

  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
};

