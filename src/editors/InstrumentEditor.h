
#pragma once

#include <qcombobox.h>
#include <qtmetamacros.h>  // for Q_OBJECT

#include "metatypes/Instrument.h"

class QWidget;

const auto MAX_COMBO_BOX_ITEMS = 10;

class InstrumentEditor : public QComboBox {
  Q_OBJECT
 public:
  explicit InstrumentEditor(QWidget* parent_pointer_input = nullptr);
  void set_instrument(const Instrument& interval);
  [[nodiscard]] auto get_instrument() const -> Instrument;
};