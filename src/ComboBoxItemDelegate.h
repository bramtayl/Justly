#pragma once
#include <QStyledItemDelegate>
#include <QComboBox>

class ComboBoxItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    std::vector<std::unique_ptr<const QString>>& instrument_pointers;
    QComboBox dummy;

    ComboBoxItemDelegate(std::vector<std::unique_ptr<const QString>>& instrument_pointers, QObject *parent = nullptr);
    ~ComboBoxItemDelegate();

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const;
};
