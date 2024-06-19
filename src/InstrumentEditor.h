
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/Instrument.h"  // IWYU pragma: keep
#include "justly/global.h"      // for JUSTLY_EXPORT

class QWidget;

class JUSTLY_EXPORT InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument* value READ value WRITE setValue USER true)
 public:
  explicit InstrumentEditor(QWidget* = nullptr, bool = true);
  void setValue(const Instrument* new_value);
  [[nodiscard]] auto value() const -> const Instrument*;
};
