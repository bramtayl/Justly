#pragma once

#include <nlohmann/json.hpp>

#include "rational/Rational.hpp"

struct PercussionInstrument;
struct PercussionSet;

template <typename T> class QList;
namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Percussion {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer = nullptr;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;

  Percussion() = default;
  explicit Percussion(const nlohmann::json &json_percussion);
  void copy_columns_from(const Percussion& template_percussion, int left_column, int right_column);
  [[nodiscard]] auto to_json(int left_column,
                             int right_column) const -> nlohmann::json;
};

[[nodiscard]] auto get_percussions_schema() -> nlohmann::json;
[[nodiscard]] auto get_percussions_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
