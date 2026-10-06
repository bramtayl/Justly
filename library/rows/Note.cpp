#include "rows/Note.hpp"

auto Note::get_field_name(const NoteField field) -> const char* {
  switch (field) {
    case NoteField::voice_name:
      return "Voice";
    case NoteField::beats:
      return "Beats";
    case NoteField::velocity_ratio:
      return "Velocity ratio";
    case NoteField::words:
      return "Words";
  }
  Q_UNREACHABLE();
}

auto Note::get_field(const NoteField field) const -> QVariant {
  switch (field) {
    case NoteField::voice_name:
      return voice_name;
    case NoteField::beats:
      return QVariant::fromValue(beats);
    case NoteField::velocity_ratio:
      return QVariant::fromValue(velocity_ratio);
    case NoteField::words:
      return words;
  }
  Q_UNREACHABLE();
}

void Note::set_field(const NoteField field, const QVariant& new_value) {
  switch (field) {
    case NoteField::voice_name:
      voice_name = variant_to<QString>(new_value);
      break;
    case NoteField::beats:
      beats = variant_to<Rational>(new_value);
      break;
    case NoteField::velocity_ratio:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    case NoteField::words:
      words = variant_to<QString>(new_value);
      break;
  }
}

void Note::field_to_xml(xmlNode& node, const NoteField field) const {
  switch (field) {
    case NoteField::voice_name:
      set_xml_string(node, "voice_name", voice_name.toStdString());
      break;
    case NoteField::beats:
      maybe_add_rational_to_xml(node, "beats", beats);
      break;
    case NoteField::velocity_ratio:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    case NoteField::words:
      maybe_add_qstring_to_xml(node, "words", words);
      break;
  }
}

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
