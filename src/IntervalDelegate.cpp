#include "IntervalDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>
#include <qrect.h>        // for QRect
#include <qvariant.h>     // for QVariant
#include <qwidget.h>      // for QWidget

#include "Interval.h"
#include "IntervalEditor.h"  // for IntervalEditor

IntervalDelegate::IntervalDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto IntervalDelegate::createEditor(QWidget *parent_pointer,
                                    const QStyleOptionViewItem & /*option*/,
                                    const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  QPointer<IntervalEditor> interval_editor_pointer = new IntervalEditor(parent_pointer);
  return interval_editor_pointer;
}

// set the data in the editor_pointer based on whats currently in the box
void IntervalDelegate::setEditorData(QWidget *editor_pointer,
                                     const QModelIndex &index) const {
  QPointer<IntervalEditor> interval_editor_pointer =
      qobject_cast<IntervalEditor *>(editor_pointer);
  interval_editor_pointer->set_interval(
      qvariant_cast<Interval>(index.data(Qt::DisplayRole)));
}

// move data from the editor_pointer to the model
void IntervalDelegate::setModelData(QWidget *editor_pointer, QAbstractItemModel *model,
                                    const QModelIndex &index) const {
  model->setData(index,
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
