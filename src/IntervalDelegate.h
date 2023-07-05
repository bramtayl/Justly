#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

class QAbstractItemModel;
class QModelIndex;
class QObject;
class QWidget;

class IntervalDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit IntervalDelegate(QObject *parent_pointer = nullptr);

  [[nodiscard]] auto createEditor(QWidget *parent_pointer,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
      -> QWidget * override;
  void setEditorData(QWidget *editor_pointer, const QModelIndex &index) const override;
  void setModelData(QWidget *editor_pointer, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor_pointer, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};