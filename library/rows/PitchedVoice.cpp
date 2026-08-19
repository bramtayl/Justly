#include "rows/PitchedVoice.hpp"

#include <QtCore/QString>
#include <QtCore/QTypeInfo>
#include <QtCore/QVariant>
#include <QtCore/QtAssert>
#include <libxml/parser.h>

#include "cell_types/Rational.hpp"
#include "column_numbers/PitchedVoiceColumn.hpp"
#include "other/helpers.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"

PitchedVoice::PitchedVoice() : Voice() {
  program = "Grand Piano";
}

auto PitchedVoice::get_pitched() -> const char * { return "pitched"; }

auto PitchedVoice::is_pitched() -> bool { return true; }

auto PitchedVoice::get_preview_midi_number() -> short {
  return MIDDLE_C_MIDI;
}

void PitchedVoice::from_xml(xmlNode &node) {
  auto *field_pointer = xmlFirstElementChild(&node);
  while (field_pointer != nullptr) {
    auto &field_node = get_reference(field_pointer);
    const auto field_name = get_xml_name(field_node);
    if (field_name == "name") {
      name = get_qstring_content(field_node);
    } else if (field_name == "instrument") {
      program = get_qstring_content(field_node);
    } else if (field_name == "velocity_ratio") {
      set_rational_from_xml(velocity_ratio, field_node);
    } else {
      Q_UNREACHABLE();
    }
    field_pointer = xmlNextElementSibling(field_pointer);
  }
}

auto PitchedVoice::get_clipboard_schema() -> const char * {
  return "pitched_voice_clipboard.xsd";
}

auto PitchedVoice::get_xml_field_name() -> const char * {
  return "pitched_voice";
}

auto PitchedVoice::get_number_of_columns() -> int {
  return static_cast<int>(PitchedVoiceColumn::number_of_pitched_voice_columns);
}

auto PitchedVoice::get_column_name(int column_number) -> const char * {
  switch (static_cast<PitchedVoiceColumn>(column_number)) {
  case PitchedVoiceColumn::number_of_pitched_voice_columns:
    Q_UNREACHABLE();
  case PitchedVoiceColumn::pitched_voice_name_column:
    return "Name";
  case PitchedVoiceColumn::pitched_voice_instrument_column:
    return "Instrument";
  case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
    return "Velocity ratio";
  }
  Q_UNREACHABLE();
}

auto PitchedVoice::get_cells_mime() -> const char * {
  return "application/prs.pitched_voice_cells+xml";
}

auto PitchedVoice::is_column_editable(int /*column_number*/) -> bool {
  return true;
}

auto PitchedVoice::get_data(const int column_number) const -> QVariant {
  switch (static_cast<PitchedVoiceColumn>(column_number)) {
  case PitchedVoiceColumn::number_of_pitched_voice_columns:
    Q_UNREACHABLE();
  case PitchedVoiceColumn::pitched_voice_name_column:
    return name;
  case PitchedVoiceColumn::pitched_voice_instrument_column:
    return program;
  case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
    return QVariant::fromValue(velocity_ratio);
  }
  Q_UNREACHABLE();
}

void PitchedVoice::set_data(const int column_number, const QVariant &new_value) {
  switch (static_cast<PitchedVoiceColumn>(column_number)) {
  case PitchedVoiceColumn::number_of_pitched_voice_columns:
    Q_UNREACHABLE();
  case PitchedVoiceColumn::pitched_voice_name_column:
    name = variant_to<QString>(new_value);
    break;
  case PitchedVoiceColumn::pitched_voice_instrument_column:
    program = variant_to<QString>(new_value);
    break;
  case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
    velocity_ratio = variant_to<Rational>(new_value);
    break;
  }
}

void PitchedVoice::copy_column_from(const PitchedVoice &template_row,
                                    const int column_number) {
  switch (static_cast<PitchedVoiceColumn>(column_number)) {
  case PitchedVoiceColumn::number_of_pitched_voice_columns:
    Q_UNREACHABLE();
  case PitchedVoiceColumn::pitched_voice_name_column:
    name = template_row.name;
    break;
  case PitchedVoiceColumn::pitched_voice_instrument_column:
    program = template_row.program;
    break;
  case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
    velocity_ratio = template_row.velocity_ratio;
    break;
  }
}

void PitchedVoice::column_to_xml(xmlNode &node, const int column_number) const {
  switch (static_cast<PitchedVoiceColumn>(column_number)) {
  case PitchedVoiceColumn::number_of_pitched_voice_columns:
    Q_UNREACHABLE();
  case PitchedVoiceColumn::pitched_voice_name_column:
    maybe_add_qstring_to_xml(node, "name", name);
    break;
  case PitchedVoiceColumn::pitched_voice_instrument_column:
    maybe_add_qstring_to_xml(node, "instrument", program);
    break;
  case PitchedVoiceColumn::pitched_voice_velocity_ratio_column:
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
    break;
  }
}

void PitchedVoice::to_xml(xmlNode &node) const {
  maybe_add_qstring_to_xml(node, "name", name);
  maybe_add_qstring_to_xml(node, "instrument", program);
  maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
}
