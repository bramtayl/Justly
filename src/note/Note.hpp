#pragma once

#include <QString>
#include <QVariant>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "rational/Rational.hpp"

struct Instrument;

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

const auto NUMBER_OF_NOTE_COLUMNS = 5;

[[nodiscard]] auto to_note_column(int column) -> NoteColumn;

struct Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  QString words;

  Note() = default;
  explicit Note(const nlohmann::json &json_note);

  [[nodiscard]] auto get_data(int column_number) const -> QVariant;
  void set_data_directly(int column, const QVariant &new_value);

  void copy_columns_from(const Note &template_row, int left_column,
                         int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json;
};

[[nodiscard]] auto get_notes_schema() -> nlohmann::json;
[[nodiscard]] auto
get_notes_cells_validator() -> const nlohmann::json_schema::json_validator &;
