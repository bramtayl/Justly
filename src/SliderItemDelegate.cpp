#include "SliderItemDelegate.h"
#include "ShowSlider.h"

#include <QSlider>

SliderItemDelegate::SliderItemDelegate(int minimum, int maximum, const QString& suffix, QObject *parent) : QStyledItemDelegate(parent), minimum(minimum), maximum(maximum), suffix(suffix)
{
}


SliderItemDelegate::~SliderItemDelegate()
{
}


QWidget *SliderItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Create the combobox and populate it
    return new ShowSlider(minimum, maximum, suffix, parent);
}

// set the data in the editor based on whats currently in the box
void SliderItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ShowSlider *slider_pointer = qobject_cast<ShowSlider *>(editor);
    Q_ASSERT(slider_pointer);
    slider_pointer->slider.setValue(index.data(Qt::DisplayRole).toInt());
}

// move data from the editor to the model
void SliderItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ShowSlider *slider_pointer = qobject_cast<ShowSlider *>(editor);
    Q_ASSERT(slider_pointer);
    model->setData(index, slider_pointer->slider.value(), Qt::EditRole);
}

QSize SliderItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    return dummy.sizeHint();

}

