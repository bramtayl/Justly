#pragma once

#include "rows/Voice.hpp"

static const auto DEFAULT_MIDI_NUMBER = 57;

struct UnpitchedVoice : Voice {
  UnpitchedVoice();

  short midi_number = DEFAULT_MIDI_NUMBER;

  [[nodiscard]] static auto get_pitched() -> const char*;

  [[nodiscard]] static auto is_pitched() -> bool;

  [[nodiscard]] auto get_preview_midi_number() const -> short;

  void from_xml(xmlNode& node) override;

  [[nodiscard]] static auto get_clipboard_schema() -> const char*;

  [[nodiscard]] static auto get_xml_field_name() -> const char*;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;

  [[nodiscard]] static auto get_cells_mime() -> const char*;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;

  void set_data(int column_number, const QVariant& new_value) override;

  void copy_column_from(const UnpitchedVoice& template_row, int column_number);

  void column_to_xml(xmlNode& node, int column_number) const override;

  void to_xml(xmlNode& node) const override;
};
