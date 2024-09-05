#pragma once

#include <QString>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "percussion_set/PercussionSet.hpp"
#include "rational/Rational.hpp"

struct Instrument;
struct PercussionInstrument;

struct Percussion {
  const PercussionSet *percussion_set_pointer;
  const PercussionInstrument *percussion_instrument_pointer;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  Percussion();
};

[[nodiscard]] auto
percussions_to_json(const std::vector<Percussion> &percussions,
                    size_t first_percussion_number,
                    size_t number_of_percussions) -> nlohmann::json;

void json_to_percussions(std::vector<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions,
                         size_t number_of_percussions);