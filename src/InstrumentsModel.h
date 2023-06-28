#pragma once

#include <qabstractitemmodel.h>  // for QAbstractListModel, QModelIndex
#include <qtmetamacros.h>        // for Q_OBJECT
#include <qvariant.h>            // for QVariant

#include <vector>  // for vector

class Instrument;
class QObject;

class InstrumentsModel : public QAbstractListModel {
  Q_OBJECT
 public:
  const std::vector<Instrument> &instruments;
  const bool include_empty;

  explicit InstrumentsModel(const std::vector<Instrument> &instruments,
                            bool include_empty,
                            QObject *parent_input = nullptr);
  [[nodiscard]] auto data(const QModelIndex &index, int role) const
      -> QVariant override;
  [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
      -> int override;
};