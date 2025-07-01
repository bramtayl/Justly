#pragma once

#include "cell_types/Rational.hpp"

static const auto MILLISECONDS_PER_MINUTE = 60000;

struct Row {
  Rational beats;
  Rational velocity_ratio;
  QString words;

  virtual ~Row() = default;
  virtual void from_xml(xmlNode &node) = 0;

  [[nodiscard]] virtual auto get_data(int column_number) const -> QVariant = 0;

  virtual void set_data(int column, const QVariant &new_value) = 0;
  virtual void column_to_xml(xmlNode &node, int column_number) const = 0;
  virtual void to_xml(xmlNode &chord_node) const = 0;
};

template <typename SubRow>
concept RowInterface =
    std::derived_from<SubRow, Row> &&
    requires(SubRow target_row, const SubRow &template_row, xmlNode &node,
             int column_number) {
      {
        target_row.copy_column_from(template_row, column_number)
      } -> std::same_as<void>;
      { SubRow::get_number_of_columns() } -> std::same_as<int>;
      { SubRow::get_column_name(column_number) } -> std::same_as<const char *>;
      { SubRow::get_clipboard_schema() } -> std::same_as<const char *>;
      { SubRow::get_xml_field_name() } -> std::same_as<const char *>;
      { SubRow::get_cells_mime() } -> std::same_as<const char *>;
      { SubRow::is_column_editable(column_number) } -> std::same_as<bool>;
    };

template <RowInterface SubRow>
static void xml_to_rows(QList<SubRow> &new_rows, xmlNode &node) {
  auto *xml_row_pointer = xmlFirstElementChild(&node);
  while (xml_row_pointer != nullptr) {
    SubRow child_row;
    child_row.from_xml(get_reference(xml_row_pointer));
    new_rows.push_back(std::move(child_row));
    xml_row_pointer = xmlNextElementSibling(xml_row_pointer);
  }
}

template <RowInterface SubRow>
static void maybe_set_xml_rows(xmlNode &node, const char *const array_name,
                               const QList<SubRow> &rows) {
  if (!rows.empty()) {
    auto &rows_node = get_new_child(node, array_name);
    for (const auto &row : rows) {
      row.to_xml(get_new_child(rows_node, SubRow::get_xml_field_name()));
    }
  }
}

static inline void maybe_set_xml_qstring(xmlNode &node,
                                         const char *const field_name,
                                         const QString &words) {
  if (!words.isEmpty()) {
    set_xml_string(node, field_name, words.toStdString());
  }
}

[[nodiscard]] static inline auto get_qstring_content(const xmlNode &node) {
  return QString(get_c_string_content(node));
}

[[nodiscard]] static inline auto get_duration_in_milliseconds(const double beats_per_minute,
                                                  const Row &row) {
  return rational_to_double(row.beats) * MILLISECONDS_PER_MINUTE / beats_per_minute;
}
