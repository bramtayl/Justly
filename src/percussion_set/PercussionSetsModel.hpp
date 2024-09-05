#pragma once

#include <QAbstractItemModel>
#include <QVariant>

class QObject;

struct PercussionSetsModel : public QAbstractListModel {
  explicit PercussionSetsModel(QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent*/) const -> int override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
};
