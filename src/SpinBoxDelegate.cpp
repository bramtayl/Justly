#include "SpinBoxDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>
#include <qrect.h>     // for QRect
#include <qspinbox.h>  // for QSpinBox
#include <qvariant.h>  // for QVariant
#include <qwidget.h>   // for QWidget

SpinBoxDelegate::SpinBoxDelegate(int minimum, int maximum, QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer), minimum(minimum), maximum(maximum) {}

auto SpinBoxDelegate::createEditor(QWidget *parent_pointer,
                                   const QStyleOptionViewItem & /*option*/,
                                   const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  QPointer<QSpinBox> spin_box_pointer = new QSpinBox(parent_pointer);
  spin_box_pointer->setMinimum(minimum);
  spin_box_pointer->setMaximum(maximum);
  return spin_box_pointer;
}

// set the data in the editor_pointer based on whats currently in the box
void SpinBoxDelegate::setEditorData(QWidget *editor_pointer,
                                    const QModelIndex &index) const {
  qobject_cast<QSpinBox *>(editor_pointer)->setValue(index.data(Qt::DisplayRole).toInt());
}

// move data from the editor_pointer to the model
void SpinBoxDelegate::setModelData(QWidget *editor_pointer, QAbstractItemModel *model,
                                   const QModelIndex &index) const {
  auto *spin_box_pointer = qobject_cast<QSpinBox *>(editor_pointer);
  model->setData(index, spin_box_pointer->value(), Qt::EditRole);
}

void SpinBoxDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &option,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = option.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
