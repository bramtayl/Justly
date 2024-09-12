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

[[nodiscard]] auto chords_to_json(const QList<Chord> &chords,
                                  qsizetype first_chord_number,
                                  qsizetype number_of_chords,
                                  bool include_children = true) -> nlohmann::json;

void json_to_chords(QList<Chord> &new_chords,
                    const nlohmann::json &json_chords, qsizetype number_of_chords);