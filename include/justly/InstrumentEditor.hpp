
#pragma once

#include <QComboBox>
#include <QObject>

#include "justly/Instrument.hpp" // IWYU pragma: keep
#include "justly/JUSTLY_EXPORT.hpp"

class QWidget;

class JUSTLY_EXPORT InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument *value READ value WRITE setValue USER true)
public:
  explicit InstrumentEditor(QWidget *parent_pointer_input = nullptr,
                            bool include_empty = true);

  [[nodiscard]] auto value() const -> const Instrument *;
  void setValue(const Instrument *new_value);
};
