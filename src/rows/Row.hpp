#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

// In addition to the following, a sub-row should have the following methods:
// SubRow::SubRow(const nlohmann::json& json_row);
// [[nodiscard]] static auto get_column_name(int column_number) -> QString;
// [[nodiscard]] static auto get_number_of_columns() -> int;
// void SubRow::copy_columns_from(const SubRow &template_row, int left_column,
//                        int right_column);
struct Row {
  virtual ~Row() = default;
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
      [](const nlohmann::json &json_chord) { return SubRow(json_chord); });
}

template <std::derived_from<Row> SubRow>
void json_to_rows(QList<SubRow> &rows, const nlohmann::json &json_rows) {
  partial_json_to_rows(rows, json_rows, static_cast<int>(json_rows.size()));
}

template <std::derived_from<Row> SubRow>
void json_field_to_rows(QList<SubRow> &rows, nlohmann::json json_object,
                        const char *field) {
  if (json_object.contains(field)) {
    const auto &json_rows = json_object[field];
    json_to_rows(rows, json_rows);
  }
}

template <std::derived_from<Row> SubRow>
static void add_rows_to_json(nlohmann::json &json_chord,
                             const QList<SubRow> &rows,
                             const char *field_name) {
  if (!rows.empty()) {
    nlohmann::json json_rows = nlohmann::json::array();
    std::transform(
        rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
        [](const SubRow &row) -> nlohmann::json { return row.to_json(); });
    json_chord[field_name] = std::move(json_rows);
  }
}