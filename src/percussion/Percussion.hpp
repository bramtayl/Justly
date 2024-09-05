#pragma once

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <QList> // IWYU pragma: keep

#include "percussion_set/PercussionSet.hpp" // IWYU pragma: keep
#include "rational/Rational.hpp"

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
percussions_to_json(const QList<Percussion> &percussions,
                    qsizetype first_percussion_number,
                    qsizetype number_of_percussions) -> nlohmann::json;

void json_to_percussions(QList<Percussion> &new_percussions,
                         const nlohmann::json &json_percussions,
                         qsizetype number_of_percussions);