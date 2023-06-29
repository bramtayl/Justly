#pragma once

#include <qpointer.h>             // for QPointer
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

class QAbstractItemModel;
class QModelIndex;
class QObject;
class QWidget;

const auto MAX_COMBO_BOX_ITEMS = 10;

class ComboBoxDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  QPointer<QAbstractItemModel> model_pointer;

  explicit ComboBoxDelegate(const QPointer<QAbstractItemModel> &model_pointer,
                            QObject *parent = nullptr);

  [[nodiscard]] auto createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
      -> QWidget * override;
  void setEditorData(QWidget *editor, const QModelIndex &index) const override;
  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};
