#include "ComboBoxDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qcombobox.h>           // for QComboBox
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>            // for QPointer
#include <qrect.h>               // for QRect
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

ComboBoxDelegate::ComboBoxDelegate(
    const QPointer<QAbstractItemModel> &model_pointer_input, QObject *parent_pointer)
    : model_pointer(model_pointer_input), QStyledItemDelegate(parent_pointer) {}

auto ComboBoxDelegate::createEditor(QWidget *parent_pointer,
                                    const QStyleOptionViewItem & /*style_info*/,
                                    const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  QPointer<QComboBox> combo_box_pointer = new QComboBox(parent_pointer);
  combo_box_pointer->setModel(model_pointer);
  combo_box_pointer->setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  // force scrollbar for combo box
  combo_box_pointer->setStyleSheet("combobox-popup: 0;");
  return combo_box_pointer;
}

// set the data in the editor_pointer based on whats currently in the box
void ComboBoxDelegate::setEditorData(QWidget *editor_pointer,
                                     const QModelIndex &index) const {
  // get the index of the text in the combobox that matches the current value of
  // the item
  qobject_cast<QComboBox *>(editor_pointer)->setCurrentText(index.data(Qt::DisplayRole).toString());
}

// move data from the editor_pointer to the model
void ComboBoxDelegate::setModelData(QWidget *editor_pointer, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  model->setData(index, qobject_cast<QComboBox *>(editor_pointer)->currentText(),
                 Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &style_info,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = style_info.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
