#include "SpinBoxItemDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget
#include <qlayout.h>

#include "Utilities.h"

SpinBoxItemDelegate::SpinBoxItemDelegate(int minimum, int maximum,
                                         QObject *parent)
    : QStyledItemDelegate(parent), minimum(minimum), maximum(maximum) {}

auto SpinBoxItemDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem & /*option*/,
                                       const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  auto *spin_box_pointer = new QSpinBox(parent);
  spin_box_pointer->setMinimum(minimum);
  spin_box_pointer->setMaximum(maximum);
  return spin_box_pointer;
}

// set the data in the editor based on whats currently in the box
void SpinBoxItemDelegate::setEditorData(QWidget *editor,
                                        const QModelIndex &index) const {
  auto *spin_box_pointer = qobject_cast<QSpinBox *>(editor);
  spin_box_pointer->setValue(index.data(Qt::DisplayRole).toInt());
}

// move data from the editor to the model
void SpinBoxItemDelegate::setModelData(QWidget *editor,
                                       QAbstractItemModel *model,
                                       const QModelIndex &index) const {
  auto *spin_box_pointer = qobject_cast<QSpinBox *>(editor);
  model->setData(index, spin_box_pointer->value(), Qt::EditRole);
}

void SpinBoxItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor -> setGeometry(frame);
}
