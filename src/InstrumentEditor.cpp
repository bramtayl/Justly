#include "justly/InstrumentEditor.h"

#include <qcombobox.h>   // for QComboBox
#include <qnamespace.h>  // for EditRole
#include <qvariant.h>    // for QVariant

#include <memory>                         // for make_unique, __unique_ptr_t

#include "justly/Instrument.h"
#include "src/InstrumentsModel.h"  // for InstrumentsModel

const auto MAX_COMBO_BOX_ITEMS = 10;

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

auto InstrumentEditor::get_instrument_pointer() const -> const Instrument* {
  return currentData(Qt::EditRole).value<const Instrument*>();
}

void InstrumentEditor::set_instrument_pointer(const Instrument* new_value) {
  setCurrentIndex(findData(QVariant::fromValue(new_value), Qt::EditRole));
}
