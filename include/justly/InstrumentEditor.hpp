
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qsize.h>         // for QSize
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY
#include <qwidget.h>

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
