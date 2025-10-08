#pragma once

#include "column_numbers/PitchedVoiceColumn.hpp"
#include "rows/Row.hpp"
#include "rows/Voice.hpp"

struct PitchedVoice : Voice {
  PitchedVoice() : Voice() {
    program = "Grand Piano";
  }

  [[nodiscard]] static auto get_description() { return "pitched voice"; }

  void from_xml(xmlNode &node) override {
    auto *field_pointer = xmlFirstElementChild(&node);
    while (field_pointer != nullptr) {
      auto &field_node = get_reference(field_pointer);
      const auto field_name = get_xml_name(field_node);
      if (field_name == "name") {
        name = get_qstring_content(field_node);
      } else if (field_name == "instrument") {
        program = get_qstring_content(field_node);
      } else {
        Q_ASSERT(false);
      }
      field_pointer = xmlNextElementSibling(field_pointer);
    }
  }

  [[nodiscard]] static auto get_clipboard_schema() -> const char * {
    return "pitched_voice_clipboard.xsd";
  };

  [[nodiscard]] static auto get_xml_field_name() -> const char * {
    return "pitched_voice";
  };

  [[nodiscard]] static auto get_number_of_columns() -> int {
    return number_of_pitched_voice_columns;
  };

  [[nodiscard]] static auto get_column_name(int column_number) {
    switch (column_number) {
    case pitched_voice_name_column:
      return "Name";
    case pitched_voice_instrument_column:
      return "Instrument";
    default:
      Q_ASSERT(false);
      return "";
    };
  }

  [[nodiscard]] static auto get_cells_mime() {
    return "application/prs.pitched_voice_cells+xml";
  }

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool {
    return true;
  }

  [[nodiscard]] auto
  get_data(const int column_number) const -> QVariant override {
    switch (column_number) {
    case pitched_voice_name_column:
      return name;
    case pitched_voice_instrument_column:
      return program;
    default:
      Q_ASSERT(false);
      return {};
    }
  }

  void set_data(const int column_number, const QVariant &new_value) override {
    switch (column_number) {
    case pitched_voice_name_column:
      // TODO(brandon): check that name doesn't already exist and isn't empty
      name = variant_to<QString>(new_value);
      break;
    case pitched_voice_instrument_column:
      program = variant_to<QString>(new_value);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void copy_column_from(const PitchedVoice &template_row,
                        const int column_number) {
    switch (column_number) {
    case pitched_voice_name_column:
      name = template_row.name;
      break;
    case pitched_voice_instrument_column:
      program = template_row.program;
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void column_to_xml(xmlNode &node, const int column_number) const override {
    switch (column_number) {
    case pitched_voice_name_column:
      maybe_add_qstring_to_xml(node, "name", name);
      break;
    case pitched_voice_instrument_column:
      maybe_add_qstring_to_xml(node, "instrument", program);
      break;
    default:
      Q_ASSERT(false);
    }
  }

  void to_xml(xmlNode &node) const override {
    maybe_add_qstring_to_xml(node, "name", name);
    maybe_add_qstring_to_xml(node, "instrument", program);
  }
};
