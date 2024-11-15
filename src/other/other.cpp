#include "other/other.hpp"

#include <cmath>

auto get_number_schema(const char *type, int minimum,
                       int maximum) -> nlohmann::json {
  return nlohmann::json(
      {{"type", type}, {"minimum", minimum}, {"maximum", maximum}});
}

auto to_int(double value) -> int { return static_cast<int>(std::round(value)); }