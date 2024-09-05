#pragma once

#include <QAbstractItemModel>
#include <QVariant>
#include <Qt>

class QObject;

struct PercussionInstrumentsModel : public QAbstractListModel {
  explicit PercussionInstrumentsModel(QObject *parent_pointer_input = nullptr);

  [[nodiscard]] auto
  rowCount(const QModelIndex & /*parent*/) const -> int override;
  [[nodiscard]] auto
  flags(const QModelIndex & /*index*/) const -> Qt::ItemFlags override;
  [[nodiscard]] auto data(const QModelIndex &index,
                          int role) const -> QVariant override;
};
