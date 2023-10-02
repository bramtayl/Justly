
#pragma once

#include <qcombobox.h>     // for QComboBox
#include <qtmetamacros.h>  // for Q_OBJECT

class Instrument;
class QWidget;

const auto MAX_COMBO_BOX_ITEMS = 10;

class InstrumentEditor : public QComboBox {
  Q_OBJECT
 public:
  explicit InstrumentEditor(QWidget* parent_pointer_input = nullptr);
  void setValue(const Instrument* instrument_pointer);
  void set_value_no_signals(const Instrument* instrument_pointer);
  [[nodiscard]] auto value() const -> const Instrument*;
};
