#include "rows/Row.hpp"

void maybe_add_qstring_to_xml(xmlNode& node, const char* const field_name,
                              const QString& words) {
  if (!words.isEmpty()) {
    set_xml_string(node, field_name, words.toStdString());
  }
}

auto get_duration_in_milliseconds(const double beats_per_minute,
                                  const double beats_double) -> double {
  static const auto MILLISECONDS_PER_MINUTE = 60000;
  return beats_double * MILLISECONDS_PER_MINUTE / beats_per_minute;
}
