
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qtmetamacros.h>  // for Q_OBJECT

#include "metatypes/Instrument.h"

class QWidget;

class InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(
      const Instrument* instrument_pointer READ value WRITE setValue USER true)
 public:
  explicit InstrumentEditor(QWidget* parent_pointer_input = nullptr,
                            bool include_empty = true);
  void setValue(const Instrument* new_value);
  [[nodiscard]] auto value() const -> const Instrument*;
};
