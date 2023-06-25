#include "ComboBoxItemDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qsizepolicy.h>         // for QSizePolicy, QSizePolicy::MinimumExp...
#include <qstring.h>             // for QString
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include "Utilities.h"  // for fill_combo_box

ComboBoxItemDelegate::ComboBoxItemDelegate(
    std::vector<std::unique_ptr<const QString>> &instrument_pointers,
    QObject *parent)
    : instrument_pointers(instrument_pointers), QStyledItemDelegate(parent) {
}

auto ComboBoxItemDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem & /*option*/,
                                        const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  auto *combo_box_pointer = new QComboBox(parent);
  combo_box_pointer->setSizePolicy(QSizePolicy::MinimumExpanding,
                                   QSizePolicy::MinimumExpanding);
  fill_combo_box(*combo_box_pointer, instrument_pointers, true);
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
  auto *combo_box_pointer = qobject_cast<QComboBox *>(editor);
  model->setData(index, combo_box_pointer->currentText(), Qt::EditRole);
}


void ComboBoxItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor -> setGeometry(frame);
}

