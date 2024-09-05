
#pragma once

#include <QComboBox>
#include <QObject>

#include "percussion_instrument/PercussionInstrument.hpp" // IWYU pragma: keep

class QWidget;

struct PercussionInstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(
      const PercussionInstrument *value READ value WRITE setValue USER true)

public:
  explicit PercussionInstrumentEditor(QWidget *parent_pointer_input = nullptr);

  [[nodiscard]] auto value() const -> const PercussionInstrument *;
  void setValue(const PercussionInstrument *new_value);
};
