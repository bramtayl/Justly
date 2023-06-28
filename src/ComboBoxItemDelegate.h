#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT
#include <QPointer>

#include "Instrument.h"
#include "InstrumentsModel.h"
#include <memory>  // for unique_ptr
#include <vector>  // for vector
class QAbstractItemModel;
class QModelIndex;
class QObject;
class QString;
class QWidget;

class ComboBoxItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  QPointer<QAbstractItemModel> model_pointer;

  explicit ComboBoxItemDelegate(
      const QPointer<QAbstractItemModel> &model_pointer,
      QObject *parent = nullptr);

  [[nodiscard]] auto createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
      -> QWidget * override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
