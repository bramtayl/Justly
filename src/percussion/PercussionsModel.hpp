#pragma once

#include <QString>
#include <QVariant>
#include <QtGlobal>

#include "justly/PercussionColumn.hpp"
#include "other/ItemModel.hpp"

struct Percussion;
class QObject;
class QModelIndex;
class QUndoStack;
template <typename T> class QList;

[[nodiscard]] auto to_percussion_column(int column) -> PercussionColumn;

struct PercussionsModel : public ItemModel {
  QList<Percussion> *percussions_pointer = nullptr;
  QUndoStack *const undo_stack_pointer;

  explicit PercussionsModel(QUndoStack *undo_stack_pointer_input,
                            QObject *parent_pointer = nullptr);
  // override functions
  [[nodiscard]] auto
  rowCount(const QModelIndex &parent_index) const -> int override;
  [[nodiscard]] auto
  columnCount(const QModelIndex &parent) const -> int override;

  [[nodiscard]] auto get_column_name(int column_number) const -> QString override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
  [[nodiscard]] auto setData(const QModelIndex &index,
                             const QVariant &new_value,
                             int role) -> bool override;
};

void insert_percussions(PercussionsModel &percussions_model,
                        qsizetype first_percussion_number,
                        const QList<Percussion> &new_percussions);
void remove_percussions(PercussionsModel &percussions_model,
                        qsizetype first_percussion_number,
                        qsizetype number_of_percussions);
