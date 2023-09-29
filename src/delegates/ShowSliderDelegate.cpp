#include "delegates/ShowSliderDelegate.h"

#include <qabstractitemmodel.h>   // for QAbstractItemModel, QModelIndex
#include <qnamespace.h>           // for DisplayRole
#include <qobject.h>              // for qobject_cast, QObject (ptr only)
#include <qrect.h>                // for QRect
#include <qslider.h>              // for QSlider
#include <qstring.h>              // for QString
#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qvariant.h>             // for QVariant
#include <qwidget.h>              // for QWidget

#include <gsl/pointers>  // for not_null
#include <utility>       // for move

#include "editors/ShowSlider.h"        // for ShowSlider
#include "metatypes/SuffixedNumber.h"  // for SuffixedNumber

ShowSliderDelegate::ShowSliderDelegate(int minimum, int maximum, QString suffix,
                                       QObject *parent_pointer)
    : QStyledItemDelegate(parent_pointer),
      minimum(minimum),
      maximum(maximum),
      suffix(std::move(suffix)) {}

auto ShowSliderDelegate::createEditor(QWidget *parent_pointer,
                                      const QStyleOptionViewItem & /*option*/,
                                      const QModelIndex & /*index*/) const
    -> QWidget * {
  // Create the combobox and populate it
  return new ShowSlider(minimum, maximum, suffix, parent_pointer);
}

// move data from the model to the editor_pointer
void ShowSliderDelegate::setEditorData(QWidget *editor_pointer,
                                       const QModelIndex &index) const {
  qobject_cast<ShowSlider *>(editor_pointer)
      ->setValue(static_cast<int>(
          index.data(Qt::DisplayRole).value<SuffixedNumber>().number));
}

// move data from the editor_pointer to the model
void ShowSliderDelegate::setModelData(QWidget *editor_pointer,
                                      QAbstractItemModel *model,
                                      const QModelIndex &index) const {
  model->setData(
      index,
      QVariant::fromValue(SuffixedNumber(
          qobject_cast<ShowSlider *>(editor_pointer)->value(),
          "")));
}

void ShowSliderDelegate::updateEditorGeometry(
    QWidget *editor_pointer, const QStyleOptionViewItem &option,
    const QModelIndex & /*index*/) const {
  QRect frame_copy = option.rect;
  frame_copy.setSize(editor_pointer->sizeHint());
  editor_pointer->setGeometry(frame_copy);
}
