#pragma once

#include <QtGlobal>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

template <typename T> class QList;

struct Percussion {
  const PercussionSet *percussion_set_pointer =
      get_percussion_set_pointer("Standard");
  const PercussionInstrument *percussion_instrument_pointer =
      get_percussion_instrument_pointer("Tambourine");
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
};

[[nodiscard]] auto get_percussions_schema() -> nlohmann::json;
[[nodiscard]] auto get_percussions_cells_validator()
    -> const nlohmann::json_schema::json_validator &;
[[nodiscard]] auto
percussions_to_json(const QList<Percussion> &percussions,
                    qsizetype first_percussion_number,
                    qsizetype number_of_percussions) -> nlohmann::json;
void json_to_percussions(QList<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions,
                         qsizetype number_of_percussions);