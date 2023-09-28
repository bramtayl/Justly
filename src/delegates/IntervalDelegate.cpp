#include "delegates/IntervalDelegate.h"

#include <gsl/pointers>
#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qrect.h>               // for QRect
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qvariant.h>            // for QVariant
#include <qwidget.h>              // for QWidget

#include "editors/IntervalEditor.h"  // for IntervalEditor
#include "metatypes/Interval.h"

IntervalDelegate::IntervalDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto IntervalDelegate::createEditor(QWidget *parent_pointer,
                                    const QStyleOptionViewItem & /*option*/,
                                    const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  return new IntervalEditor(parent_pointer);
}

// set the data in the editor_pointer based on whats currently in the box
void IntervalDelegate::setEditorData(QWidget *editor_pointer,
                                     const QModelIndex &index) const {
  qobject_cast<IntervalEditor *>(editor_pointer)
      ->set_interval(qvariant_cast<Interval>(index.data(Qt::DisplayRole)));
}

// move data from the editor_pointer to the model
void IntervalDelegate::setModelData(QWidget *editor_pointer,
                                    QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  model->setData(
      index,
      QVariant::fromValue(
          qobject_cast<IntervalEditor *>(editor_pointer)->get_interval()),
      Qt::EditRole);
}

void IntervalDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &option,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = option.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
