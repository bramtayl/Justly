#pragma once

#include <QVariant>
#include <nlohmann/json.hpp>

#include "justly/PercussionColumn.hpp"
#include "rational/Rational.hpp"
#include "rows/Row.hpp"

struct PercussionInstrument;
struct PercussionSet;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_PERCUSSION_COLUMNS = 4;

[[nodiscard]] auto to_percussion_column(int column) -> PercussionColumn;

struct Percussion : Row {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Rational beats;
  Rational velocity_ratio;

  void from_json(const nlohmann::json &json_percussion) override;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data_directly(int column, const QVariant &new_value) override;

  void copy_columns_from(const Percussion &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json override;
};

[[nodiscard]] auto get_percussions_schema() -> nlohmann::json;
[[nodiscard]] auto get_percussions_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
