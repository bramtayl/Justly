#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>
#include <QtGlobal>

#include "other/ItemModel.hpp"
#include "justly/PercussionColumn.hpp"

struct Percussion;
class QUndoStack;
class QWidget;
template <typename T> class QList;

[[nodiscard]] auto to_percussion_column(int column) -> PercussionColumn;

struct PercussionsModel : public ItemModel {
  QWidget *const parent_pointer;
  QList<Percussion>* percussions_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit PercussionsModel(QUndoStack *undo_stack_pointer_input,
                            QWidget *parent_pointer_input = nullptr);
  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex &parent_index) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto headerData(int column, Qt::Orientation orientation,
                                int role) const -> QVariant override;
  [[nodiscard]] auto
  flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
};

void insert_percussions(PercussionsModel& percussions_model,
                        qsizetype first_percussion_number,
                        const QList<Percussion> &new_percussions);
void remove_percussions(PercussionsModel& percussions_model,
                        qsizetype first_percussion_number,
                        qsizetype number_of_percussions);
