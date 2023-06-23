#pragma once

#include <qsize.h>                // for QSize
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

#include "IntervalEditor.h"

class QAbstractItemModel;
class QModelIndex;
class QObject;
class QWidget;


class IntervalDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  IntervalEditor dummy;

  explicit IntervalDelegate(QObject *parent = nullptr);

  [[nodiscard]] auto createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
      -> QWidget * override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  [[nodiscard]] auto sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const -> QSize override;
};
