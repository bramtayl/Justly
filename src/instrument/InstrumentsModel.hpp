#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QVariant>

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT

public:
  explicit InstrumentsModel(QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent*/) const -> int override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
};
