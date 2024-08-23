
#pragma once

#include <QComboBox>
#include <QObject>

#include "instrument/Instrument.hpp" // IWYU pragma: keep

class QWidget;

class InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer_input = nullptr,
                            bool include_empty = true);

  [[nodiscard]] auto value() const -> const Instrument *;
  void setValue(const Instrument *new_value);
};
