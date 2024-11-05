#pragma once

#include <QList>
#include <QVariant>
#include <concepts>
#include <iterator>
#include <nlohmann/json.hpp>
#include <utility>

// In addition to the following, a sub-row should have the following methods:
// SubRow::SubRow(const nlohmann::json& json_row);
// [[nodiscard]] static auto is_column_editable(int column_number) -> bool; (optional)
// [[nodiscard]] static auto get_column_name(int column_number) -> QString;
// [[nodiscard]] static auto get_number_of_columns() -> int;
// void SubRow::copy_columns_from(const SubRow &template_row, int left_column,
//                        int right_column);
struct Row {
  virtual ~Row() = default;
  [[nodiscard]] static auto is_column_editable(int column_number) -> bool;
  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;
  virtual void set_data_directly(int column, const QVariant &new_value) = 0;
  [[nodiscard]] virtual auto
  columns_to_json(int left_column,
                  int right_column) const -> nlohmann::json = 0;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json = 0;
};

template <std::derived_from<Row> SubRow>
void partial_json_to_rows(QList<SubRow> &new_rows,
                          const nlohmann::json &json_rows, int number_of_rows) {
  std::transform(
      json_rows.cbegin(), json_rows.cbegin() + static_cast<int>(number_of_rows),
      std::back_inserter(new_rows),
      [](const nlohmann::json &json_row) { return SubRow(json_row); });
}

template <std::derived_from<Row> SubRow>
void json_to_rows(QList<SubRow> &rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(rows, json_rows, static_cast<int>(json_rows.size()));
}

template <std::derived_from<Row> SubRow>
auto json_field_to_rows(nlohmann::json json_object,
                        const char *field) -> QList<SubRow> {
  if (json_object.contains(field)) {
    QList<SubRow> rows;
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
    return rows;
  }
  return {};
}

template <std::derived_from<Row> SubRow>
static void add_rows_to_json(nlohmann::json &json_chord,
                             const QList<SubRow> &rows,
                             const char *field_name) {
  if (!rows.empty()) {
    nlohmann::json json_rows;
    std::transform(
        rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
        [](const SubRow &row) -> nlohmann::json { return row.to_json(); });
    json_chord[field_name] = std::move(json_rows);
  }
}