#include "other/other.hpp"

#include <QWidget>

void prevent_compression(QWidget &widget) {
  widget.setMinimumSize(widget.minimumSizeHint());
}