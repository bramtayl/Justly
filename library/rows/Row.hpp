#pragma once

#include "other/helpers.hpp"
#include "xml/XMLChildren.hpp"

template <typename SubRow>
concept RowInterface =
    requires(SubRow row, const SubRow& const_row, xmlNode& node,
             int column_number, const QVariant& new_value) {
      row.from_xml(node);
      { const_row.get_data(column_number) } -> std::same_as<QVariant>;
      row.set_data(column_number, new_value);
      const_row.column_to_xml(node, column_number);
      { SubRow::get_number_of_columns() } -> std::same_as<int>;
      { SubRow::get_column_name(column_number) } -> std::same_as<const char*>;
      { SubRow::get_clipboard_schema() } -> std::same_as<const char*>;
      { SubRow::get_xml_field_name() } -> std::same_as<const char*>;
      { SubRow::get_cells_mime() } -> std::same_as<const char*>;
    };

// every column is editable, unless the row has its own is_column_editable,
// e.g. a chord's notes, which are edited in their own table
template <RowInterface SubRow>
[[nodiscard]] auto column_is_editable(const int column_number) -> bool {
  if constexpr (requires { SubRow::is_column_editable(column_number); }) {
    return SubRow::is_column_editable(column_number);
  } else {
    return true;
  }
}

// copies through get_data/set_data, unless the row has its own
// copy_column_from for columns that don't round-trip, e.g. a chord's notes
template <RowInterface SubRow>
void copy_column(SubRow& target_row, const SubRow& template_row,
                 const int column_number) {
  if constexpr (requires {
                  target_row.copy_column_from(template_row, column_number);
                }) {
    target_row.copy_column_from(template_row, column_number);
  } else {
    target_row.set_data(column_number, template_row.get_data(column_number));
  }
}

// writes columns [left_column, right_column] of each row, in column order,
// by default every column
template <RowInterface SubRow>
void rows_to_xml(xmlNode& rows_node, const QList<SubRow>& rows,
                 const int left_column = 0,
                 const int right_column = SubRow::get_number_of_columns() - 1) {
  for (const auto& row : rows) {
    auto& row_node = get_new_child(rows_node, SubRow::get_xml_field_name());
    for (auto column_number = left_column; column_number <= right_column;
         column_number++) {
      row.column_to_xml(row_node, column_number);
    }
  }
}

// reads at most max_rows rows
template <RowInterface SubRow>
void xml_to_rows(QList<SubRow>& new_rows, xmlNode& node,
                 const int max_rows = std::numeric_limits<int>::max()) {
  for (auto& xml_row : get_xml_children(node) | std::views::take(max_rows)) {
    SubRow child_row;
    child_row.from_xml(xml_row);
    new_rows.push_back(std::move(child_row));
  }
}

template <RowInterface SubRow>
void maybe_set_xml_rows(xmlNode& node, const char* const array_name,
                        const QList<SubRow>& rows) {
  if (!rows.empty()) {
    rows_to_xml(get_new_child(node, array_name), rows);
  }
}

void maybe_add_qstring_to_xml(xmlNode& node, const char* field_name,
                              const QString& words);
