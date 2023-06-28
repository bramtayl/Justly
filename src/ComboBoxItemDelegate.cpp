#include "ComboBoxItemDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qcombobox.h>           // for QComboBox
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>            // for QPointer
#include <qrect.h>               // for QRect
#include <qsizepolicy.h>         // for QSizePolicy, QSizePolicy::MinimumExp...
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget
#include "Utilities.h"

ComboBoxItemDelegate::ComboBoxItemDelegate(
    const QPointer<QAbstractItemModel>& model_pointer_input,
    QObject *parent)
    : model_pointer(model_pointer_input), QStyledItemDelegate(parent) {
}

auto ComboBoxItemDelegate::createEditor(QWidget *parent,
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
void ComboBoxItemDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const {
  auto *combo_box_pointer = qobject_cast<QComboBox *>(editor);
  // get the index of the text in the combobox that matches the current value of
  // the item
  const QString current_text = index.data(Qt::DisplayRole).toString();
  const int combo_box_index = combo_box_pointer->findText(current_text);
  // if it is valid, adjust the combobox
  if (combo_box_index >= 0) {
    combo_box_pointer->setCurrentIndex(combo_box_index);
  }
}

// move data from the editor to the model
void ComboBoxItemDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const {
  model->setData(index, qobject_cast<QComboBox *>(editor)->currentText(), Qt::EditRole);
}


void ComboBoxItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor -> setGeometry(frame);
}

