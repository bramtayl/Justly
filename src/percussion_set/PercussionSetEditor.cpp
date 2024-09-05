#include "percussion_set/PercussionSetEditor.hpp"

#include <QComboBox>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "percussion_set/PercussionSet.hpp"
#include "percussion_set/PercussionSetsModel.hpp"

class QWidget;

PercussionSetEditor::PercussionSetEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  setModel(
      std::make_unique<PercussionSetsModel>(parent_pointer_input).release());
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");

  setMinimumSize(sizeHint());
}

// TODO: avoid qvariant
auto PercussionSetEditor::value() const -> const PercussionSet * {
  auto current_value = currentData(Qt::EditRole);
  Q_ASSERT(current_value.canConvert<const PercussionSet *>());
  return current_value.value<const PercussionSet *>();
}

void PercussionSetEditor::setValue(const PercussionSet *new_value) {
  auto index = findData(QVariant::fromValue(new_value), Qt::EditRole);
  Q_ASSERT(index >= 0);
  setCurrentIndex(index);
}
