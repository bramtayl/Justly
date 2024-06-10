
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qmetatype.h>     // for qRegisterMetaType, qRegisterNormalizedMet...
#include <qtmetamacros.h>  // for Q_OBJECT, Q_PROPERTY

#include "justly/Instrument.h" // IWYU pragma: keep

class QWidget;

Q_DECLARE_METATYPE(const Instrument*);

class InstrumentEditor : public QComboBox {
  Q_OBJECT
  Q_PROPERTY(const Instrument* value READ value
                 WRITE setValue USER true)
 public:
  explicit InstrumentEditor(QWidget* = nullptr, bool = true);
  void setValue(const Instrument* new_value);
  [[nodiscard]] auto value() const -> const Instrument*;
};
