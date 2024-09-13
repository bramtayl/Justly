#pragma once

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>
#include <QList>

#include "interval/Interval.hpp"
#include "note/Note.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

struct Chord {
  QList<Note> notes;
  QList<Percussion> percussions;

  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};
