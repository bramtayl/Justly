#include "delegates/InstrumentDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qrect.h>               // for QRect
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qvariant.h>            // for QVariant
#include <qwidget.h>              // for QWidget

#include <memory>  // for unique_ptr, make_unique

#include "editors/InstrumentEditor.h"  // for InstrumentEditor
#include "metatypes/Instrument.h"

InstrumentDelegate::InstrumentDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto InstrumentDelegate::createEditor(
    QWidget *parent_pointer, const QStyleOptionViewItem & /*style_info*/,
    const QModelIndex & /*index*/) const -> QWidget * {
  // Create the combobox and populate it
  return std::make_unique<InstrumentEditor>(parent_pointer).release();
}

// set the data in the editor_pointer based on whats currently in the box
void InstrumentDelegate::setEditorData(QWidget *editor_pointer,
                                       const QModelIndex &index) const {
  // get the index of the text in the combobox that matches the current value of
  // the item
  qobject_cast<InstrumentEditor *>(editor_pointer)
      ->set_instrument(qvariant_cast<Instrument>(index.data(Qt::DisplayRole)));
}

// move data from the editor_pointer to the model
void InstrumentDelegate::setModelData(QWidget *editor_pointer,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  model->setData(
      index,
      QVariant::fromValue(
          qobject_cast<InstrumentEditor *>(editor_pointer)->get_instrument()),
      Qt::EditRole);
}

void InstrumentDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &style_info,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = style_info.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
