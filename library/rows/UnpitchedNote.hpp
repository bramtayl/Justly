#pragma once

#include "rows/Note.hpp"

struct UnpitchedNote : Note {
  void from_xml(xmlNode& node);

  [[nodiscard]] static auto get_clipboard_schema() -> const char*;

  [[nodiscard]] static auto get_xml_field_name() -> const char*;

  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;

  [[nodiscard]] static auto get_cells_mime() -> const char*;

  [[nodiscard]] static auto get_pitched() -> const char*;

  [[nodiscard]] static auto is_column_editable(int /*column_number*/) -> bool;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant;

  void set_data(int column_number, const QVariant& new_value);

  void column_to_xml(xmlNode& node, int column_number) const;
};
