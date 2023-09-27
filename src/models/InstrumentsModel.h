#pragma once

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <vector>  // for vector

class Instrument;
class QObject;

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
 private:
  const std::vector<Instrument> *instruments_pointer;
  bool include_empty;
 public:
  explicit InstrumentsModel(bool include_empty,
                            QObject *parent_pointer_input = nullptr);
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent) const -> int override;
};
