
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormalizedMeta...
#include <qtmetamacros.h>  // for Q_OBJECT

#include "src/Instrument.h"

class QWidget;

Q_DECLARE_METATYPE(const Instrument*);

class InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument* instrument_pointer READ get_instrument_pointer
                 WRITE set_instrument_pointer USER true)
 public:
  explicit InstrumentEditor(QWidget* = nullptr, bool = true);
  void set_instrument_pointer(const Instrument* new_value);
  [[nodiscard]] auto get_instrument_pointer() const -> const Instrument*;
};
