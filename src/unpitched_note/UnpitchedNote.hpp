#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "justly/UnpitchedNoteColumn.hpp"
#include "rational/Rational.hpp"
#include "rows/Row.hpp"

struct PercussionInstrument;
struct PercussionSet;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_UNPITCHED_NOTE_COLUMNS = 5;

[[nodiscard]] auto to_unpitched_note_column(int column) -> UnpitchedNoteColumn;

struct UnpitchedNote : Row {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  void from_json(const nlohmann::json &json_percussion) override;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data_directly(int column, const QVariant &new_value) override;

  void copy_columns_from(const UnpitchedNote &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column,
                             int right_column) const -> nlohmann::json override;
};

[[nodiscard]] auto get_unpitched_notes_schema() -> nlohmann::json;
[[nodiscard]] auto get_unpitched_notes_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
