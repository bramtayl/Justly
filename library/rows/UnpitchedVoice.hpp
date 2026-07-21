#pragma once

#include "column_numbers/UnpitchedVoiceColumn.hpp"
#include "rows/Voice.hpp"

static const auto DEFAULT_MIDI_NUMBER = 57;

struct UnpitchedVoice : Voice {
  UnpitchedVoice() : Voice() {
    program = "Standard";
  }

  short midi_number = DEFAULT_MIDI_NUMBER;

  [[nodiscard]] static auto get_pitched() { return "unpitched"; }

  [[nodiscard]] static auto is_pitched() { return false; }

  [[nodiscard]] auto get_preview_midi_number() const -> short {
    return midi_number;
  }

  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while (field_pointer != nullptr) {
      auto &field_node = get_reference(field_pointer);
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
        Q_ASSERT(false);
      }
      field_pointer = xmlNextElementSibling(field_pointer);
    }
  }

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "unpitched_voice_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_field_name() -> const char * {
    return "unpitched_voice";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_unpitched_voice_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case unpitched_voice_name_column:
      return "Name";
    case unpitched_voice_percussion_set_column:
      return "Percussion set";
    case unpitched_voice_midi_number_column:
      return "MIDI number";
    case unpitched_voice_velocity_ratio_column:
      return "Velocity ratio";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.unpitched_voice_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case unpitched_voice_name_column:
      return name;
    case unpitched_voice_percussion_set_column:
      return program;
    case unpitched_voice_midi_number_column:
      return QVariant::fromValue(midi_number);
    case unpitched_voice_velocity_ratio_column:
      return QVariant::fromValue(velocity_ratio);
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case unpitched_voice_name_column:
      name = variant_to<QString>(new_value);
      break;
    case unpitched_voice_percussion_set_column:
      program = variant_to<QString>(new_value);
      break;
    case unpitched_voice_midi_number_column:
      midi_number = variant_to<short>(new_value);
      break;
    case unpitched_voice_velocity_ratio_column:
      velocity_ratio = variant_to<Rational>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const UnpitchedVoice &template_row,
                        const int column_number) {
    switch (column_number) {
    case unpitched_voice_name_column:
      name = template_row.name;
      break;
    case unpitched_voice_percussion_set_column:
      program = template_row.program;
      break;
    case unpitched_voice_midi_number_column:
      midi_number = template_row.midi_number;
      break;
    case unpitched_voice_velocity_ratio_column:
      velocity_ratio = template_row.velocity_ratio;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case unpitched_voice_name_column:
      maybe_add_qstring_to_xml(node, "name", name);
      break;
    case unpitched_voice_percussion_set_column:
      maybe_add_qstring_to_xml(node, "percussion_set_pointer", program);
      break;
    case unpitched_voice_midi_number_column:
      set_xml_int(node, "midi_number", midi_number);
      break;
    case unpitched_voice_velocity_ratio_column:
      maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &node) const override {
    maybe_add_qstring_to_xml(node, "name", name);
    maybe_add_qstring_to_xml(node, "percussion_set_pointer", program);
    set_xml_int(node, "midi_number", midi_number);
    maybe_add_rational_to_xml(node, "velocity_ratio", velocity_ratio);
  }
};
