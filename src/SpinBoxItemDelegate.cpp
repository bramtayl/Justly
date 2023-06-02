#include "SpinBoxItemDelegate.h"

#include <QSpinBox>

SpinBoxItemDelegate::SpinBoxItemDelegate(int minimum, int maximum, QObject *parent) : QStyledItemDelegate(parent), minimum(minimum), maximum(maximum)
{
}


SpinBoxItemDelegate::~SpinBoxItemDelegate()
{
}


QWidget *SpinBoxItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Create the combobox and populate it
    QSpinBox *spin_box_pointer = new QSpinBox(parent);
    spin_box_pointer -> setMinimum(minimum);
    spin_box_pointer -> setMaximum(maximum);
    return spin_box_pointer;
}

// set the data in the editor based on whats currently in the box
void SpinBoxItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox *spin_box_pointer = qobject_cast<QSpinBox *>(editor);
    Q_ASSERT(spin_box_pointer);
    spin_box_pointer->setValue(index.data(Qt::DisplayRole).toInt());
}

// move data from the editor to the model
void SpinBoxItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *spin_box_pointer = qobject_cast<QSpinBox *>(editor);
    Q_ASSERT(spin_box_pointer);
    model->setData(index, spin_box_pointer->value(), Qt::EditRole);
}

QSize SpinBoxItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    return dummy.sizeHint();
}