#include "main/MyDelegate.h"

#include <qabstractitemmodel.h>   // for QModelIndex, QAbstractItemModel
#include <qrect.h>                // for QRect
#include <qspinbox.h>             // for QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, unique_ptr

#include "notechord/NoteChord.h"       // for beats_column, instrument_column

MyDelegate::MyDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto MyDelegate::createEditor(QWidget *parent_pointer,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const -> QWidget * {
  auto column = index.column();
  if (column == beats_column) {
    auto* spin_box_pointer = std::make_unique<QSpinBox>(parent_pointer).release();
    spin_box_pointer->setMinimum(MINIMUM_BEATS);
    spin_box_pointer->setMaximum(MAXIMUM_BEATS);
    return spin_box_pointer;
  }
  if (column == volume_percent_column) {
    auto* spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer).release();
    spin_box_pointer->setDecimals(1);
    spin_box_pointer->setMinimum(MINIMUM_VOLUME_PERCENT);
    spin_box_pointer->setMaximum(MAXIMUM_VOLUME_PERCENT);
    spin_box_pointer->setSuffix("%");
    return spin_box_pointer;
  }
  if (column == tempo_percent_column) {
    auto* spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer).release();
    spin_box_pointer->setDecimals(1);
    spin_box_pointer->setMinimum(MINIMUM_TEMPO_PERCENT);
    spin_box_pointer->setMaximum(MAXIMUM_TEMPO_PERCENT);
    spin_box_pointer->setSuffix("%");
    return spin_box_pointer;
  }
  return QStyledItemDelegate::createEditor(parent_pointer, option, index);
}

void MyDelegate::updateEditorGeometry(QWidget *editor_pointer,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex & /*index*/) const {
  QRect frame_copy = option.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
