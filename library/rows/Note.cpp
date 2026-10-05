#include "rows/Note.hpp"

void Note::note_field_from_xml(const std::string& name, xmlNode& field_node) {
  if (name == "beats") {
    set_rational_from_xml(beats, field_node);
  } else if (name == "velocity_ratio") {
    set_rational_from_xml(velocity_ratio, field_node);
  } else if (name == "words") {
    words = get_qstring_content(field_node);
  } else {
    Q_ASSERT(name == "voice_name");
    voice_name = get_qstring_content(field_node);
  }
}
