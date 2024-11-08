#pragma once

#include <QVariant>
#include <nlohmann/json.hpp>

#include "abstract_rational/interval/Interval.hpp"
#include "row/Row.hpp"

struct Instrument;

struct PitchedNote : Row {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;

  PitchedNote() = default;
  explicit PitchedNote(const nlohmann::json& json_note);
  [[nodiscard]] static auto get_column_name(int column_number) -> const char*;
  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data(int column, const QVariant &new_value) override;

  void copy_columns_from(const PitchedNote &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column,
                             int right_column) const -> nlohmann::json override;
};

[[nodiscard]] auto get_pitched_notes_schema() -> nlohmann::json;