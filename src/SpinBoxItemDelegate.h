#pragma once
#include <QStyledItemDelegate>
#include <QSpinBox>

class SpinBoxItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    const int minimum;
    const int maximum;
    QSpinBox dummy;
    SpinBoxItemDelegate(int minimum, int maximum, QObject *parent = nullptr);
    ~SpinBoxItemDelegate();

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const;
};
