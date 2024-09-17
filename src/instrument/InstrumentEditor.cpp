#include "instrument/InstrumentEditor.hpp"

#include <QComboBox>
#include <QList>
#include <iterator>
#include <memory>

#include "instrument/Instrument.hpp"
#include "instrument/InstrumentsModel.hpp"

class QWidget;

InstrumentEditor::InstrumentEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  setModel(std::make_unique<InstrumentsModel>(parent_pointer_input).release());
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");

  setMinimumSize(sizeHint());
}

auto InstrumentEditor::value() const -> const Instrument * {
  return &get_all_instruments()[currentIndex()];
}

void InstrumentEditor::setValue(const Instrument *new_value) {
  setCurrentIndex(std::distance(get_all_instruments().data(), new_value));
}
