#pragma once
#include <QStyledItemDelegate>
#include <QSlider>
#include "ShowSlider.h"

class SliderItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    const int minimum;
    const int maximum;
    const QString suffix;
    ShowSlider dummy = ShowSlider(0, 0, suffix);
    SliderItemDelegate(int minimum, int maximum, QString  suffix, QObject *parent = nullptr);
    ~SliderItemDelegate() = default;


    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    auto sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const -> QSize override;
};


