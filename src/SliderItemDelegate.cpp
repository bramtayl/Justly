#include "SliderItemDelegate.h"

#include <QtCore/qtcoreexports.h>  // for Q_ASSERT
#include <qabstractitemmodel.h>    // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>            // for DisplayRole, EditRole
#include <qobject.h>               // for qobject_cast, QObject (ptr only)
#include <qslider.h>               // for QSlider
#include <qvariant.h>              // for QVariant
#include <qwidget.h>               // for QWidget

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

// set the data in the editor based on whats currently in the box
void SliderItemDelegate::setEditorData(QWidget *editor,
                                       const QModelIndex &index) const {
  auto *slider_pointer = qobject_cast<ShowSlider *>(editor);
  Q_ASSERT(slider_pointer);
  slider_pointer->slider.setValue(index.data(Qt::DisplayRole).toInt());
}

// move datsssa from the editor to the model
void SliderItemDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  auto *slider_pointer = qobject_cast<ShowSlider *>(editor);
  Q_ASSERT(slider_pointer);
  model->setData(index, slider_pointer->slider.value(), Qt::EditRole);
}

auto SliderItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                  const QModelIndex & /*index*/) const
    -> QSize {
  return dummy.sizeHint();
}
