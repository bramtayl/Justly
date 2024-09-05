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

// TODO: avoid qvariant
auto PercussionInstrumentEditor::value() const -> const PercussionInstrument * {
  auto current_value = currentData(Qt::EditRole);
  Q_ASSERT(current_value.canConvert<const PercussionInstrument *>());
  return current_value.value<const PercussionInstrument *>();
}

void PercussionInstrumentEditor::setValue(
    const PercussionInstrument *new_value) {
  auto index = findData(QVariant::fromValue(new_value), Qt::EditRole);
  Q_ASSERT(index >= 0);
  setCurrentIndex(index);
}
