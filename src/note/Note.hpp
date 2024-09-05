#pragma once

#include <QList> // IWYU pragma: keep
#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

struct Instrument;

struct Note {
  const Instrument *instrument_pointer;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;

  Note();
};

[[nodiscard]] auto notes_to_json(const QList<Note> &notes,
                                 qsizetype first_note_number,
                                 qsizetype number_of_notes) -> nlohmann::json;

void json_to_notes(QList<Note> &new_notes,
                   const nlohmann::json &json_notes, qsizetype number_of_notes);