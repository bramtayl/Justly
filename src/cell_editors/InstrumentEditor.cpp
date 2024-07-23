#include "justly/InstrumentEditor.hpp"

#include <QComboBox>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "justly/Instrument.hpp"
#include "models/InstrumentsModel.hpp"

class QWidget;

const auto MAX_COMBO_BOX_ITEMS = 10;

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input,
                                   bool include_empty)
    : QComboBox(parent_pointer_input) {
  setModel(
      std::make_unique<InstrumentsModel>(include_empty, parent_pointer_input)
          .release());
  setMaxVisibleItems(MAX_COMBO_BOX_ITEMS);
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");

  setMinimumSize(sizeHint());
}

auto InstrumentEditor::value() const -> const Instrument * {
  auto current_value = currentData(Qt::EditRole);
  Q_ASSERT(current_value.canConvert<const Instrument *>());
  return current_value.value<const Instrument *>();
}

void InstrumentEditor::setValue(const Instrument *new_value) {
  auto index = findData(QVariant::fromValue(new_value), Qt::EditRole);
  Q_ASSERT(index >= 0);
  setCurrentIndex(index);
}
