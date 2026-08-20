#include "rows/UnpitchedVoice.hpp"
#include "column_numbers/UnpitchedVoiceColumn.hpp"

UnpitchedVoice::UnpitchedVoice() : Voice() { program = "Standard"; }

auto UnpitchedVoice::get_pitched() -> const char* { return "unpitched"; }

auto UnpitchedVoice::is_pitched() -> bool { return false; }

auto UnpitchedVoice::get_preview_midi_number() const -> short {
  return midi_number;
}

void UnpitchedVoice::from_xml(xmlNode& node) {
  auto* field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto& field_node = get_reference(field_pointer);
    const auto field_name = get_xml_name(field_node);
    if (field_name == "name") {
      name = get_qstring_content(field_node);
    } else if (field_name == "percussion_set_pointer") {
      program = get_qstring_content(field_node);
    } else if (field_name == "midi_number") {
      midi_number = static_cast<short>(xml_to_int(field_node));
    } else if (field_name == "velocity_ratio") {
      set_rational_from_xml(velocity_ratio, field_node);
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

auto UnpitchedVoice::get_clipboard_schema() -> const char* {
  return "unpitched_voice_clipboard.xsd";
}

auto UnpitchedVoice::get_xml_field_name() -> const char* {
  return "unpitched_voice";
}

auto UnpitchedVoice::get_number_of_columns() -> int {
  return static_cast<int>(
      UnpitchedVoiceColumn::number_of_unpitched_voice_columns);
}

auto UnpitchedVoice::get_column_name(int column_number) -> const char* {
  switch (static_cast<UnpitchedVoiceColumn>(column_number)) {
    case UnpitchedVoiceColumn::number_of_unpitched_voice_columns:
      Q_UNREACHABLE();
    case UnpitchedVoiceColumn::unpitched_voice_name_column:
      return "Name";
    case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
      return "Percussion set";
    case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
      return "MIDI number";
    case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
      return "Velocity ratio";
  }
  Q_UNREACHABLE();
}

auto UnpitchedVoice::get_cells_mime() -> const char* {
  return "application/prs.unpitched_voice_cells+xml";
}

auto UnpitchedVoice::is_column_editable(int /*column_number*/) -> bool {
  return true;
}

auto UnpitchedVoice::get_data(const int column_number) const -> QVariant {
  switch (static_cast<UnpitchedVoiceColumn>(column_number)) {
    case UnpitchedVoiceColumn::number_of_unpitched_voice_columns:
      Q_UNREACHABLE();
    case UnpitchedVoiceColumn::unpitched_voice_name_column:
      return name;
    case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
      return program;
    case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
      return QVariant::fromValue(midi_number);
    case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
  }
  Q_UNREACHABLE();
}

void UnpitchedVoice::set_data(const int column_number,
                              const QVariant& new_value) {
  switch (static_cast<UnpitchedVoiceColumn>(column_number)) {
    case UnpitchedVoiceColumn::number_of_unpitched_voice_columns:
      Q_UNREACHABLE();
    case UnpitchedVoiceColumn::unpitched_voice_name_column:
      name = variant_to<QString>(new_value);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
      program = variant_to<QString>(new_value);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
      midi_number = variant_to<short>(new_value);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
  }
}

void UnpitchedVoice::copy_column_from(const UnpitchedVoice& template_row,
                                      const int column_number) {
  switch (static_cast<UnpitchedVoiceColumn>(column_number)) {
    case UnpitchedVoiceColumn::number_of_unpitched_voice_columns:
      Q_UNREACHABLE();
    case UnpitchedVoiceColumn::unpitched_voice_name_column:
      name = template_row.name;
      break;
    case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
      program = template_row.program;
      break;
    case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
      midi_number = template_row.midi_number;
      break;
    case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
  }
}

void UnpitchedVoice::column_to_xml(xmlNode& node,
                                   const int column_number) const {
  switch (static_cast<UnpitchedVoiceColumn>(column_number)) {
    case UnpitchedVoiceColumn::number_of_unpitched_voice_columns:
      Q_UNREACHABLE();
    case UnpitchedVoiceColumn::unpitched_voice_name_column:
      maybe_add_qstring_to_xml(node, "name", name);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_percussion_set_column:
      maybe_add_qstring_to_xml(node, "percussion_set_pointer", program);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_midi_number_column:
      set_xml_int(node, "midi_number", midi_number);
      break;
    case UnpitchedVoiceColumn::unpitched_voice_velocity_ratio_column:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
  }
}

void UnpitchedVoice::to_xml(xmlNode& node) const {
  maybe_add_qstring_to_xml(node, "name", name);
  maybe_add_qstring_to_xml(node, "percussion_set_pointer", program);
  set_xml_int(node, "midi_number", midi_number);
  maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
}
