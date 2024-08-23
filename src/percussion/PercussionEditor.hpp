
#pragma once

#include <QComboBox>
#include <QObject>

#include "percussion/Percussion.hpp" // IWYU pragma: keep

class QWidget;

class PercussionEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Percussion *value READ value WRITE setValue USER true)
public:
  explicit PercussionEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> const Percussion *;
  void setValue(const Percussion *new_value);
};
