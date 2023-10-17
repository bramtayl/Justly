#include "main/MyDelegate.h"

#include <qabstractitemmodel.h>   // for QModelIndex, QAbstractItemModel
#include <qspinbox.h>             // for QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem

#include <memory>

#include "notechord/NoteChord.h"  // for beats_column, instrument_column

MyDelegate::MyDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto MyDelegate::create_beats_box(QWidget *parent_pointer)
    -> std::unique_ptr<QSpinBox> {
  auto spin_box_pointer = std::make_unique<QSpinBox>(parent_pointer);
  spin_box_pointer->setMinimum(MINIMUM_BEATS);
  spin_box_pointer->setMaximum(MAXIMUM_BEATS);
  return spin_box_pointer;
}

auto MyDelegate::create_volume_box(QWidget *parent_pointer)
    -> std::unique_ptr<QDoubleSpinBox> {
  auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
  spin_box_pointer->setDecimals(1);
  spin_box_pointer->setMinimum(MINIMUM_VOLUME_PERCENT);
  spin_box_pointer->setMaximum(MAXIMUM_VOLUME_PERCENT);
  spin_box_pointer->setSuffix("%");
  return spin_box_pointer;
}

auto MyDelegate::create_tempo_box(QWidget *parent_pointer)
    -> std::unique_ptr<QDoubleSpinBox> {
  auto spin_box_pointer = std::make_unique<QDoubleSpinBox>(parent_pointer);
  spin_box_pointer->setDecimals(1);
  spin_box_pointer->setMinimum(MINIMUM_VOLUME_PERCENT);
  spin_box_pointer->setMaximum(MAXIMUM_VOLUME_PERCENT);
  spin_box_pointer->setSuffix("%");
  return spin_box_pointer;
}

auto MyDelegate::createEditor(QWidget *parent_pointer,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const -> QWidget * {
  switch (static_cast<NoteChordField>(index.column())) {
    case beats_column:
      return create_beats_box(parent_pointer).release();
    case volume_percent_column:
      return create_volume_box(parent_pointer).release();
    case tempo_percent_column:
      return create_tempo_box(parent_pointer).release();
    default:
      return QStyledItemDelegate::createEditor(parent_pointer, option, index);
  }
}
