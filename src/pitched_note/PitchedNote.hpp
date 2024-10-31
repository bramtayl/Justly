#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "justly/PitchedNoteColumn.hpp"
#include "rational/Rational.hpp"
#include "rows/Row.hpp"

struct Instrument;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_PITCHED_NOTE_COLUMNS = 5;

[[nodiscard]] auto to_pitched_note_column(int column) -> PitchedNoteColumn;

struct PitchedNote : Row {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  PitchedNote() = default;
  explicit PitchedNote(const nlohmann::json& json_note);
  [[nodiscard]] static auto get_column_name(int column_number) -> QString;
  [[nodiscard]] static auto get_number_of_columns() -> int;

  [[nodiscard]] auto get_data(int column_number) const -> QVariant override;
  void set_data_directly(int column, const QVariant &new_value) override;

  void copy_columns_from(const PitchedNote &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json() const -> nlohmann::json override;
  [[nodiscard]] auto columns_to_json(int left_column,
                             int right_column) const -> nlohmann::json override;
};

[[nodiscard]] auto get_pitched_notes_schema() -> nlohmann::json;
[[nodiscard]] auto get_pitched_notes_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
