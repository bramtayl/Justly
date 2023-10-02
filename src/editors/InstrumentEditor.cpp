#include "InstrumentEditor.h"

#include <qcombobox.h>   // for QComboBox
#include <qnamespace.h>  // for DisplayRole
#include <qvariant.h>    // for QVariant

#include <memory>  // for make_unique, __unique_ptr_t

#include "metatypes/Instrument.h"     // for Instrument
#include "models/InstrumentsModel.h"  // for InstrumentsModel

class QWidget;

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  auto *const model_pointer =
      std::make_unique<InstrumentsModel>(true, parent_pointer_input).release();
  setModel(model_pointer);
  setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");
}

auto InstrumentEditor::value() const -> const Instrument * {
  return currentData(Qt::DisplayRole).value<const Instrument *>();
}

void InstrumentEditor::setValue(const Instrument* instrument_pointer) {
  setCurrentIndex(findData(QVariant::fromValue(instrument_pointer), Qt::DisplayRole));
}

void InstrumentEditor::set_value_no_signals(const Instrument *instrument_pointer) {
  blockSignals(true);
  setValue(instrument_pointer);
  blockSignals(false);
}
