#include "instrument/InstrumentEditor.hpp"

#include <QComboBox>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "instrument/Instrument.hpp"
#include "instrument/InstrumentsModel.hpp"

class QWidget;

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  setModel(
      std::make_unique<InstrumentsModel>(parent_pointer_input)
          .release());
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
