#pragma once

#include <QtWidgets/QLineEdit>

// a line edit that asks for enough width to comfortably type words or names
struct TextEditor : QLineEdit {
  explicit TextEditor(QWidget* parent_pointer);

  [[nodiscard]] auto sizeHint() const -> QSize override;
};
