#include "ComboBoxDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qcombobox.h>           // for QComboBox
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>            // for QPointer
#include <qrect.h>               // for QRect
#include <qsizepolicy.h>         // for QSizePolicy, QSizePolicy::MinimumE...
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include "utilities.h"  // for MAX_COMBO_BOX_ITEMS

ComboBoxDelegate::ComboBoxDelegate(
    const QPointer<QAbstractItemModel> &model_pointer_input, QObject *parent)
    : model_pointer(model_pointer_input), QStyledItemDelegate(parent) {}

auto ComboBoxDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem & /*option*/,
                                    const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  QPointer<QComboBox> combo_box_pointer = new QComboBox(parent);
  combo_box_pointer->setSizePolicy(QSizePolicy::MinimumExpanding,
                                   QSizePolicy::MinimumExpanding);
  combo_box_pointer->setModel(model_pointer);
  combo_box_pointer->setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  return combo_box_pointer;
}

// set the data in the editor based on whats currently in the box
void ComboBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const {
  // get the index of the text in the combobox that matches the current value of
  // the item
  set_combo_box(*(qobject_cast<QComboBox *>(editor)),
                index.data(Qt::DisplayRole).toString());
}

// move data from the editor to the model
void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  model->setData(index, qobject_cast<QComboBox *>(editor)->currentText(),
                 Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex & /*index*/) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor->setGeometry(frame);
}
