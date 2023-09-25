#include "delegates/InstrumentDelegate.h"

#include <memory>                // for unique_ptr, make_unique

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qcombobox.h>           // for QComboBox
#include <qobject.h>                  // for qobject_cast, QObject (ptr only)
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qrect.h>               // for QRect
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include "models/InstrumentsModel.h"

InstrumentDelegate::InstrumentDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto InstrumentDelegate::createEditor(QWidget *parent_pointer,
                                    const QStyleOptionViewItem & /*style_info*/,
                                    const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  auto* const combo_box_pointer = std::make_unique<QComboBox>(parent_pointer).release();
  auto* const model_pointer = std::make_unique<InstrumentsModel>(true, parent_pointer).release();
  combo_box_pointer->setModel(model_pointer);
  combo_box_pointer->setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  // force scrollbar for combo box
  combo_box_pointer->setStyleSheet("combobox-popup: 0;");
  return combo_box_pointer;
}

// set the data in the editor_pointer based on whats currently in the box
void InstrumentDelegate::setEditorData(QWidget *editor_pointer,
                                     const QModelIndex &index) const {
  // get the index of the text in the combobox that matches the current value of
  // the item
  qobject_cast<QComboBox *>(editor_pointer)
      ->setCurrentText(index.data(Qt::DisplayRole).toString());
}

// move data from the editor_pointer to the model
void InstrumentDelegate::setModelData(QWidget *editor_pointer,
                                    QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  model->setData(index,
                 qobject_cast<QComboBox *>(editor_pointer)->currentText(),
                 Qt::EditRole);
}

void InstrumentDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &style_info,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = style_info.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
