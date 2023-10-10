#include "InstrumentEditor.h"

#include <qcombobox.h>   // for QComboBox
#include <qnamespace.h>  // for EditRole
#include <qvariant.h>    // for QVariant

#include <memory>  // for make_unique, __unique_ptr_t

#include "metatypes/Instrument.h"
#include "models/InstrumentsModel.h"  // for InstrumentsModel

const auto MAX_COMBO_BOX_ITEMS = 10;

class QWidget;

InstrumentEditor::InstrumentEditor(QWidget* parent_pointer_input,
                                   bool include_empty)
    : QComboBox(parent_pointer_input) {
  setModel(
      std::make_unique<InstrumentsModel>(include_empty, parent_pointer_input)
          .release());
  setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");
}

auto InstrumentEditor::value() const -> const Instrument* {
  return currentData(Qt::EditRole).value<const Instrument*>();
}

void InstrumentEditor::setValue(const Instrument* new_value) {
  setCurrentIndex(findData(QVariant::fromValue(new_value), Qt::EditRole));
}
