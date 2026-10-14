#include "cell_editors/TextEditor.hpp"

#include <algorithm>

TextEditor::TextEditor(QWidget* const parent_pointer)
    : QLineEdit(parent_pointer) {
  setFrame(false);
}

auto TextEditor::sizeHint() const -> QSize {
  static const auto MINIMUM_WIDTH = 200;
  auto result = QLineEdit::sizeHint();
  result.setWidth(std::max(result.width(), MINIMUM_WIDTH));
  return result;
}
