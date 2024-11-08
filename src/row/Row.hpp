#pragma once

#include <QString>
#include <QVariant>
#include <QtGlobal>
#include <concepts>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

#include "abstract_rational/rational/Rational.hpp"
#include "named/Named.hpp"

template <typename T> class QList;

// In addition to the following, a sub-row should have the following methods:
// SubRow::SubRow(const nlohmann::json& json_row);
// [[nodiscard]] static auto is_column_editable(int column_number) -> bool;
// (optional)
// [[nodiscard]] static auto get_column_name(int column_number) -> QString;
// [[nodiscard]] static auto get_number_of_columns() -> int;
// void SubRow::copy_columns_from(const SubRow &template_row, int left_column,
//                        int right_column);
struct Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Row() = default;
  explicit Row(const nlohmann::json &json_chord);

  virtual ~Row() = default;
  [[nodiscard]] static auto is_column_editable(int column_number) -> bool;
  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;
  virtual void set_data(int column, const QVariant &new_value) = 0;
  [[nodiscard]] virtual auto
  columns_to_json(int left_column,
                  int right_column) const -> nlohmann::json = 0;
  [[nodiscard]] virtual auto to_json() const -> nlohmann::json;
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
void add_rows_to_json(nlohmann::json &json_chord, const QList<SubRow> &rows,
                      const char *field_name) {
  if (!rows.empty()) {
    nlohmann::json json_rows;
    std::transform(
        rows.cbegin(), rows.cend(), std::back_inserter(json_rows),
        [](const SubRow &row) -> nlohmann::json { return row.to_json(); });
    json_chord[field_name] = std::move(json_rows);
  }
}

void add_words_to_json(nlohmann::json &json_row, const QString &words);

template <std::derived_from<Named> SubNamed>
void add_named_to_json(nlohmann::json &json_row,
                       const SubNamed *named_pointer) {
  if (named_pointer != nullptr) {
    json_row[SubNamed::get_field_name()] = named_pointer->name.toStdString();
  }
}

template <std::derived_from<Named> SubNamed>
[[nodiscard]] auto json_field_to_named_pointer(const nlohmann::json &json_row)
    -> const SubNamed * {
  const char *field_name = SubNamed::get_field_name();
  if (json_row.contains(field_name)) {
    const auto &json_named = json_row[field_name];
    Q_ASSERT(json_named.is_string());
    return &get_by_name<SubNamed>(
        QString::fromStdString(json_named.get<std::string>()));
  };
  return nullptr;
}
