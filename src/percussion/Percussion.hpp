#pragma once

#include <QtGlobal>
#include <cstddef>
#include <nlohmann/json.hpp>

#include "justly/PercussionColumn.hpp"
#include "rational/Rational.hpp"

struct PercussionInstrument;
struct PercussionSet;

template <typename T> class QList;
namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Percussion {
  const PercussionSet *percussion_set_pointer = nullptr;
  const PercussionInstrument *percussion_instrument_pointer =
      nullptr;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
};

[[nodiscard]] auto get_percussions_schema() -> nlohmann::json;
[[nodiscard]] auto get_percussions_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
[[nodiscard]] auto percussions_to_json(
    const QList<Percussion> &percussions, qsizetype first_percussion_number,
    qsizetype number_of_percussions,
    PercussionColumn left_percussion_column = percussion_set_column,
    PercussionColumn right_percussion_column = percussion_tempo_ratio_column)
    -> nlohmann::json;
void partial_json_to_percussions(QList<Percussion> &new_percussions,
                                 const nlohmann::json &json_percussions,
                                 size_t number_of_percussions);
void json_to_percussions(QList<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions);