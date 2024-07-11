
#pragma once

#include <QComboBox>  // for QComboBox
#include <QObject>    // for Q_OBJECT, Q_PROPERTY
#include <QSize>      // for QSize
#include <QWidget>

#include "justly/Instrument.hpp"        // IWYU pragma: keep
#include "justly/public_constants.hpp"  // for JUSTLY_EXPORT

[[nodiscard]] auto get_instrument_size() -> QSize;

class JUSTLY_EXPORT InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument* value READ value WRITE setValue USER true)
 public:
  explicit InstrumentEditor(QWidget* parent_pointer_input = nullptr,
                            bool include_empty = true);

  [[nodiscard]] auto value() const -> const Instrument*;
  void setValue(const Instrument* new_value);
};
