#include "main/MyDelegate.h"

#include <qabstractitemmodel.h>   // for QModelIndex, QAbstractItemModel
#include <qnamespace.h>           // for UserRole, EditRole
#include <qobject.h>              // for qobject_cast, QObject (ptr only)
#include <qrect.h>                // for QRect
#include <qspinbox.h>             // for QSpinBox
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qvariant.h>             // for QVariant
#include <qwidget.h>              // for QWidget

#include <memory>  // for make_unique, unique_ptr

#include "editors/InstrumentEditor.h"  // for InstrumentEditor
#include "editors/IntervalEditor.h"    // for IntervalEditor
#include "editors/ShowSlider.h"        // for ShowSlider
#include "metatypes/Instrument.h"      // for Instrument
#include "metatypes/Interval.h"        // for Interval
#include "notechord/NoteChord.h"       // for beats_column, instrument_column

MyDelegate::MyDelegate(QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer) {}

auto MyDelegate::createEditor(QWidget *parent_pointer,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const -> QWidget * {
  auto column = index.column();
  if (column == beats_column) {
    auto spin_box_pointer = std::make_unique<QSpinBox>(parent_pointer);
    spin_box_pointer->setMinimum(MINIMUM_BEATS);
    spin_box_pointer->setMaximum(MAXIMUM_BEATS);
    return spin_box_pointer.release();
  }
  if (column == interval_column) {
    return new IntervalEditor(parent_pointer);
  }
  if (column == volume_percent_column) {
    return new ShowSlider(MINIMUM_VOLUME_PERCENT, MAXIMUM_VOLUME_PERCENT, "%",
                          parent_pointer);
  }
  if (column == tempo_percent_column) {
    return new ShowSlider(MINIMUM_TEMPO_PERCENT, MAXIMUM_TEMPO_PERCENT, "%",
                          parent_pointer);
  }
  if (column == instrument_column) {
    return new InstrumentEditor(parent_pointer);
  }
  return QStyledItemDelegate::createEditor(parent_pointer, option, index);
}

// set the data in the editor_pointer based on whats currently in the box
void MyDelegate::setEditorData(QWidget *editor_pointer,
                               const QModelIndex &index) const {
  auto column = index.column();
  auto data = index.data(Qt::UserRole);
  if (column == beats_column) {
    qobject_cast<QSpinBox *>(editor_pointer)
        ->setValue(data.toInt());
  } else if (column == interval_column) {
    qobject_cast<IntervalEditor *>(editor_pointer)
        ->setValue(data.value<Interval>());
  } else if (column == volume_percent_column ||
             column == tempo_percent_column) {
    qobject_cast<ShowSlider *>(editor_pointer)
        ->setValue(data.toInt());
  } else if (column == instrument_column) {
    qobject_cast<InstrumentEditor *>(editor_pointer)
        ->setValue(data.value<const Instrument *>());
  } else {
    QStyledItemDelegate::setEditorData(editor_pointer, index);
  }
}

// move data from the editor_pointer to the model
void MyDelegate::setModelData(QWidget *editor_pointer,
                              QAbstractItemModel *model,
                              const QModelIndex &index) const {
  auto column = index.column();
  if (column == beats_column) {
    model->setData(index, qobject_cast<QSpinBox *>(editor_pointer)->value(),
                   Qt::EditRole);
  } else if (column == interval_column) {
    model->setData(index,
                   QVariant::fromValue(
                       qobject_cast<IntervalEditor *>(editor_pointer)->value()),
                   Qt::EditRole);
  } else if (column == volume_percent_column ||
             column == tempo_percent_column) {
    model->setData(index,
                   QVariant::fromValue(
                       qobject_cast<ShowSlider *>(editor_pointer)->value()),
                   Qt::EditRole);
  } else if (column == instrument_column) {
    model->setData(
        index,
        QVariant::fromValue(
            qobject_cast<InstrumentEditor *>(editor_pointer)->value()),
        Qt::EditRole);
  } else {
    QStyledItemDelegate::setModelData(editor_pointer, model, index);
  }
}

void MyDelegate::updateEditorGeometry(QWidget *editor_pointer,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex & /*index*/) const {
  QRect frame_copy = option.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
