#include "SliderItemDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>            // for QPointer
#include <qslider.h>             // for QSlider
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include <utility>  // for move

#include "ShowSlider.h"  // for ShowSlider

SliderItemDelegate::SliderItemDelegate(int minimum, int maximum, QString suffix,
                                       QObject *parent)
    : QStyledItemDelegate(parent),
      minimum(minimum),
      maximum(maximum),
      suffix(std::move(suffix)) {}

auto SliderItemDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem & /*option*/,
                                      const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  return new ShowSlider(minimum, maximum, suffix, parent);
}

// move data from the model to the editor
void SliderItemDelegate::setEditorData(QWidget *editor,
                                       const QModelIndex &index) const {
  qobject_cast<ShowSlider *>(editor)->slider_pointer->setValue(index.data(Qt::DisplayRole).toDouble());
}

// move data from the editor to the model
void SliderItemDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  model->setData(index, qobject_cast<ShowSlider *>(editor)->slider_pointer->value());
}

void SliderItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor -> setGeometry(frame);
}