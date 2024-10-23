#pragma once

#include <QString>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

struct Instrument;

template <typename T> class QList;
namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  Note() = default;
  explicit Note(const nlohmann::json &json_note);
  void copy_columns_from(const Note& template_note, int left_column, int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json;
};

[[nodiscard]] auto get_notes_schema() -> nlohmann::json;
[[nodiscard]] auto
get_notes_cells_validator() -> const nlohmann::json_schema::json_validator &;
