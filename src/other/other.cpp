#include "other/other.hpp"

#include <QWidget>
#include <cmath>

void prevent_compression(QWidget &widget) {
  widget.setMinimumSize(widget.minimumSizeHint());
}

auto get_number_schema(const char *type, const char *description, int minimum,
                       int maximum) -> nlohmann::json {
  return nlohmann::json({{"type", type},
                         {"description", description},
                         {"minimum", minimum},
                         {"maximum", maximum}});
}

auto to_int(double value) -> int {
  return static_cast<int>(std::round(value));
}