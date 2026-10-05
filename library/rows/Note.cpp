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

void Note::note_fields_to_xml(xmlNode& node) const {
  maybe_add_rational_to_xml(node, "beats", beats);
  maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
  maybe_add_qstring_to_xml(node, "words", words);
}
