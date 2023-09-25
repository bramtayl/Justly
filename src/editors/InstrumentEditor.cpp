#include "models/InstrumentsModel.h"

#include "InstrumentEditor.h"

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
    auto *const model_pointer =
      std::make_unique<InstrumentsModel>(true, parent_pointer_input).release();
    setModel(model_pointer);
    setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
    // force scrollbar for combo box
    setStyleSheet("combobox-popup: 0;");
}

auto InstrumentEditor::get_instrument() const -> Instrument {
  return qvariant_cast<Instrument>(currentData(Qt::DisplayRole));
}

void InstrumentEditor::set_instrument(const Instrument &interval) {
    setCurrentIndex(
        findData(QVariant::fromValue(interval), Qt::DisplayRole)
    );
};