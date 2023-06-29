#include "ShowSliderDelegate.h"

#include <qabstractitemmodel.h>  // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>          // for DisplayRole, EditRole
#include <qobject.h>             // for qobject_cast, QObject (ptr only)
#include <qpointer.h>            // for QPointer
#include <qrect.h>               // for QRect
#include <qslider.h>             // for QSlider
#include <qvariant.h>            // for QVariant
#include <qwidget.h>             // for QWidget

#include <utility>  // for move

#include "ShowSlider.h"  // for ShowSlider
#include "SuffixedNumber.h"

ShowSliderDelegate::ShowSliderDelegate(int minimum, int maximum, QString suffix,
                                       QObject *parent)
    : QStyledItemDelegate(parent),
      minimum(minimum),
      maximum(maximum),
      suffix(std::move(suffix)) {}

auto ShowSliderDelegate::createEditor(QWidget *parent,
                                      const QStyleOptionViewItem & /*option*/,
                                      const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  return new ShowSlider(minimum, maximum, suffix, parent);
}

// move data from the model to the editor
void ShowSliderDelegate::setEditorData(QWidget *editor,
                                       const QModelIndex &index) const {
  qobject_cast<ShowSlider *>(editor)->slider_pointer->setValue(static_cast<int>(
      qvariant_cast<SuffixedNumber>(index.data(Qt::DisplayRole)).number));
}

// move data from the editor to the model
void ShowSliderDelegate::setModelData(QWidget *editor,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  model->setData(
      index,
      QVariant::fromValue(SuffixedNumber(
          qobject_cast<ShowSlider *>(editor)->slider_pointer->value(), "")));
}

void ShowSliderDelegate::updateEditorGeometry(
    QWidget *editor, const QStyleOptionViewItem &option,
    const QModelIndex & /*index*/) const {
  QRect frame = option.rect;
  frame.setSize(editor->sizeHint());
  editor->setGeometry(frame);
}