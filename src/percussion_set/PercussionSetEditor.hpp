
#pragma once

#include <QComboBox>
#include <QObject>

#include "percussion_set/PercussionSet.hpp" // IWYU pragma: keep

class QWidget;

struct PercussionSetEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const PercussionSet *value READ value WRITE setValue USER true)
public:
  explicit PercussionSetEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> const PercussionSet *;
  void setValue(const PercussionSet *new_value);
};
