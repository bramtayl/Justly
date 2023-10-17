#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

class QModelIndex;
class QObject;
class QWidget;

class MyDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit MyDelegate(QObject * = nullptr);

  [[nodiscard]] auto createEditor(QWidget * parent_pointer,
                                  const QStyleOptionViewItem & option,
                                  const QModelIndex & index) const
      -> QWidget * override;
};
