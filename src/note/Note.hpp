#pragma once

#include <QList> // IWYU pragma: keep
#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

struct Instrument;

struct Note {
  const Instrument *instrument_pointer = get_instrument_pointer("Marimba");
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};
