#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT
class QAbstractItemModel;
class QModelIndex;
class QObject;
class QWidget;

class SpinBoxItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  const int minimum;
  const int maximum;
  explicit SpinBoxItemDelegate(int minimum, int maximum,
                               QObject *parent = nullptr);

  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                    const QModelIndex &index) const -> QWidget * override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
