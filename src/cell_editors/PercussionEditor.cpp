#include "cell_editors/PercussionEditor.hpp"

#include <QComboBox>
#include <QVariant>
#include <Qt>
#include <QtGlobal>
#include <memory>

#include "cell_values/Percussion.hpp"
#include "models/PercussionsModel.hpp"

class QWidget;

PercussionEditor::PercussionEditor(QWidget *parent_pointer_input)
    : QComboBox(parent_pointer_input) {
  setModel(
      std::make_unique<PercussionsModel>(parent_pointer_input)
          .release());
  // force scrollbar for combo box
  setStyleSheet("combobox-popup: 0;");

  setMinimumSize(sizeHint());
}

auto PercussionEditor::value() const -> const Percussion * {
  auto current_value = currentData(Qt::EditRole);
  Q_ASSERT(current_value.canConvert<const Percussion *>());
  return current_value.value<const Percussion *>();
}

void PercussionEditor::setValue(const Percussion *new_value) {
  auto index = findData(QVariant::fromValue(new_value), Qt::EditRole);
  Q_ASSERT(index >= 0);
  setCurrentIndex(index);
}
