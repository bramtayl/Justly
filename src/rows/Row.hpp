#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Row {
  virtual ~Row() = default;
  virtual void from_json(const nlohmann::json &json_row) = 0;
  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;
  virtual void set_data_directly(int column, const QVariant &new_value) = 0;
  // TODO: figure out how to add this to interface
  // virtual void copy_columns_from(const Row &template_row, int left_column,
  // int right_column) = 0;
  [[nodiscard]] virtual auto
  to_json(int left_column, int right_column) const -> nlohmann::json = 0;
};

template <std::derived_from<Row> SubRow>
auto rows_to_json(const QList<SubRow> &items, int first_row_number,
                  int number_of_notes, int left_column,
                  int right_column) -> nlohmann::json {
  nlohmann::json json_rows = nlohmann::json::array();
  std::transform(
      items.cbegin() + first_row_number,
      items.cbegin() + first_row_number + number_of_notes,
      std::back_inserter(json_rows),
      [left_column, right_column](const SubRow &row) -> nlohmann::json {
        return row.to_json(left_column, right_column);
      });
  return json_rows;
}

template <std::derived_from<Row> SubRow>
void partial_json_to_rows(QList<SubRow> &new_rows,
                          const nlohmann::json &json_rows, int number_of_rows) {
  std::transform(json_rows.cbegin(),
                 json_rows.cbegin() + static_cast<int>(number_of_rows),
                 std::back_inserter(new_rows),
                 [](const nlohmann::json &json_chord) -> SubRow {
                   SubRow row;
                   row.from_json(json_chord);
                   return row;
                 });
}

template <std::derived_from<Row> SubRow>
void json_to_rows(QList<SubRow> &new_rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(new_rows, json_rows, static_cast<int>(json_rows.size()));
}
