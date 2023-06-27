#include "IntervalDelegate.h"
#include "IntervalEditor.h"      // for IntervalEditor
#include "Interval.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>
#include <qrect.h>               // for QRect
#include <qsizepolicy.h>         // for QSizePolicy, QSizePolicy::MinimumExp...
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

IntervalDelegate::IntervalDelegate(QObject *parent) : QStyledItemDelegate(parent) {
}

auto IntervalDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem & /*option*/,
                                        const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  QPointer<IntervalEditor> interval_editor_pointer = new IntervalEditor(parent);
  interval_editor_pointer->setSizePolicy(QSizePolicy::MinimumExpanding,
                                   QSizePolicy::MinimumExpanding);
  return interval_editor_pointer;
  
}

// set the data in the editor based on whats currently in the box
void IntervalDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const {
  QPointer<IntervalEditor> interval_editor_pointer = qobject_cast<IntervalEditor *>(editor);
  Interval passing;
  passing.set_text(index.data(Qt::DisplayRole).toString());
  interval_editor_pointer->set_interval(passing);
}

// move data from the editor to the model
void IntervalDelegate::setModelData(QWidget *editor,
                                        QAbstractItemModel *model,
                                        const QModelIndex &index) const {
  model->setData(index, qobject_cast<IntervalEditor *>(editor)->get_interval().get_text(), Qt::EditRole);
}

void IntervalDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor -> setGeometry(frame);
}
