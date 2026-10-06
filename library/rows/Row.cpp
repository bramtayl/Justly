#include "rows/Row.hpp"

void maybe_add_qstring_to_xml(xmlNode& node, const char* const field_name,
                              const QString& words) {
  if (!words.isEmpty()) {
    set_xml_string(node, field_name, words.toStdString());
  }
}
