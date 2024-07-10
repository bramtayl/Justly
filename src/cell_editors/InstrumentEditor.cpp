#include "justly/InstrumentEditor.hpp"

#include <qassert.h>     // for Q_ASSERT
#include <qcombobox.h>   // for QComboBox
#include <qnamespace.h>  // for EditRole
#include <qvariant.h>    // for QVariant

#include <memory>  // for make_unique, __unique_ptr_t

#include "models/InstrumentsModel.hpp"  // for InstrumentsModel

const auto MAX_COMBO_BOX_ITEMS = 10;

auto get_instrument_size() -> QSize {
  static auto instrument_size = InstrumentEditor().sizeHint();
  return instrument_size;
};

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
  auto current_value = currentData(Qt::EditRole);
  Q_ASSERT(current_value.canConvert<const Instrument*>());
  return current_value.value<const Instrument*>();
}

void InstrumentEditor::setValue(const Instrument* new_value) {
  auto index = findData(QVariant::fromValue(new_value), Qt::EditRole);
  Q_ASSERT(index >= 0);
  setCurrentIndex(index);
}
