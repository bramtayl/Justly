#pragma once

#include <QAbstractItemModel>
#include <QVariant>

class QObject;

struct InstrumentsModel : public QAbstractListModel {
  explicit InstrumentsModel(QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent*/) const -> int override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
};
