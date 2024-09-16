#include "percussion_instrument/PercussionInstrumentEditor.hpp"

#include <QComboBox>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_instrument/PercussionInstrumentsModel.hpp"

class QWidget;

PercussionInstrumentEditor::PercussionInstrumentEditor(
    QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  setModel(std::make_unique<PercussionInstrumentsModel>(parent_pointer_input)
               .release());
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");

  setMinimumSize(sizeHint());
}

auto PercussionInstrumentEditor::value() const -> const PercussionInstrument * {
  return &get_all_percussion_instruments()[currentIndex()];
}

void PercussionInstrumentEditor::setValue(
    const PercussionInstrument *new_value) {
  setCurrentIndex(
      std::distance(get_all_percussion_instruments().data(), new_value));
}
