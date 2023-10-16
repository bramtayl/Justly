#include "InstrumentEditor.h"

#include <qcombobox.h>   // for QComboBox
#include <qnamespace.h>  // for EditRole
#include <qvariant.h>    // for QVariant
#include <qwidget.h>                  // for QWidget

#include <gsl/pointers>  // for not_null

#include "metatypes/Instrument.h"
#include "models/InstrumentsModel.h"  // for InstrumentsModel

const auto MAX_COMBO_BOX_ITEMS = 10;

InstrumentEditor::InstrumentEditor(QWidget* parent_pointer_input,
                                   bool include_empty)
    : QComboBox(parent_pointer_input) {
  setModel(
      gsl::not_null(new InstrumentsModel(include_empty, parent_pointer_input)));
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
