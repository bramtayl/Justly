#pragma once

#include <qstyleditemdelegate.h>  // for QStyledItemDelegate
#include <qstyleoption.h>         // for QStyleOptionViewItem
#include <qtmetamacros.h>         // for Q_OBJECT

#include <memory>

class QDoubleSpinBox;
class QSpinBox;
class QModelIndex;
class QObject;
class QWidget;

class MyDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit MyDelegate(QObject* = nullptr);

  [[nodiscard]] auto createEditor(QWidget* parent_pointer,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
      -> QWidget* override;
  void createColumnEditor(int column);
  static auto create_beats_box(QWidget* parent_pointer)
      -> std::unique_ptr<QSpinBox>;
  static auto create_tempo_box(QWidget* parent_pointer)
      -> std::unique_ptr<QDoubleSpinBox>;
  static auto create_volume_box(QWidget* parent_pointer)
      -> std::unique_ptr<QDoubleSpinBox>;
};
