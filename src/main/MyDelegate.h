#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT
#include <qvariant.h>             // for QVariant

class QAbstractItemModel;
class QModelIndex;
class QObject;
class QWidget;

class MyDelegate : public QStyledItemDelegate {
  Q_OBJECT
 private:
  [[nodiscard]] auto get_model_data(QWidget *editor_pointer,
                                    const QModelIndex &index) -> QVariant;

 public:
  explicit MyDelegate(QObject *parent_pointer = nullptr);

  [[nodiscard]] auto createEditor(QWidget *parent_pointer,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
      -> QWidget * override;
  void setEditorData(QWidget *editor_pointer,
                     const QModelIndex &index) const override;
  void setModelData(QWidget *editor_pointer, QAbstractItemModel *model,
                    const QModelIndex &index) const override;
  void updateEditorGeometry(QWidget *editor_pointer,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const override;
};
