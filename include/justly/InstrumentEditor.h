
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormalizedMet...
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/macros.h"  // for NO_MOVE_COPY
#include "justly/Instrument.h"

class QWidget;
struct Instrument;

Q_DECLARE_METATYPE(const Instrument*);

class InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument* instrument_pointer READ get_instrument_pointer
                 WRITE set_instrument_pointer USER true)
 public:
  explicit InstrumentEditor(QWidget* = nullptr, bool = true);
  NO_MOVE_COPY(InstrumentEditor);
  void set_instrument_pointer(const Instrument* new_value);
  [[nodiscard]] auto get_instrument_pointer() const -> const Instrument*;
};
