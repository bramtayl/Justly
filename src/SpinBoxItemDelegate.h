#pragma once

#include <qsize.h>                // for QSize
#include <qspinbox.h>             // for QSpinBox
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
  QSpinBox dummy;
  SpinBoxItemDelegate(int minimum, int maximum, QObject *parent = nullptr);
  ~SpinBoxItemDelegate();

  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  auto sizeHint(const QStyleOptionViewItem &option,
                const QModelIndex &index) const -> QSize override;
};
