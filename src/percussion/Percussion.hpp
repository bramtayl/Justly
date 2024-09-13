#pragma once

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <QList> // IWYU pragma: keep

#include "percussion_instrument/PercussionInstrument.hpp"
#include "percussion_set/PercussionSet.hpp" // IWYU pragma: keep
#include "rational/Rational.hpp"

struct PercussionInstrument;

struct Percussion {
  const PercussionSet *percussion_set_pointer = get_percussion_set_pointer("Standard");
  const PercussionInstrument *percussion_instrument_pointer = get_percussion_instrument_pointer("Tambourine");
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};
