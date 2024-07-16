#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QVariant>
#include <Qt>

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
  const bool include_empty;

public:
  explicit InstrumentsModel(bool include_empty_input,
                            QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent*/) const -> int override;
  [[nodiscard]] auto
  flags(const QModelIndex &index) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
};
