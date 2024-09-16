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

auto PercussionSetEditor::value() const -> const PercussionSet * {
  return &get_all_percussion_sets()[currentIndex()];
}

void PercussionSetEditor::setValue(const PercussionSet *new_value) {
  setCurrentIndex(std::distance(get_all_percussion_sets().data(), new_value));
}
