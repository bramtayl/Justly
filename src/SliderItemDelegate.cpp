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
  auto *slider_pointer = qobject_cast<ShowSlider *>(editor);
  auto string_value = index.data(Qt::DisplayRole).toString();
  auto suffix_size = suffix.size();
  slider_pointer->slider_pointer->setValue(static_cast<int>(
      string_value.remove(string_value.size() - suffix_size, suffix_size)
          .toDouble()));
}

// move data from the editor to the model
void SliderItemDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  auto *slider_pointer = qobject_cast<ShowSlider *>(editor);
  model->setData(index, 1.0 * slider_pointer->slider_pointer->value(),
                 Qt::EditRole);
}

auto SliderItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                  const QModelIndex & /*index*/) const
    -> QSize {
  return dummy.sizeHint();
}
