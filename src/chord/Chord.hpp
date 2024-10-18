#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>
#include <cstddef>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "note/Note.hpp"
#include "percussion/Percussion.hpp"
#include "rational/Rational.hpp"

namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Chord {
  QList<Note> notes;
  QList<Percussion> percussions;

  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};

[[nodiscard]] auto get_chords_schema() -> nlohmann::json;
[[nodiscard]] auto
get_chords_cells_validator() -> const nlohmann::json_schema::json_validator &;

[[nodiscard]] auto chords_to_json(const QList<Chord> &chords,
                                  qsizetype first_chord_number,
                                  qsizetype number_of_chords) -> nlohmann::json;
void partial_json_to_chords(QList<Chord> &new_chords,
                            const nlohmann::json &json_chords,
                            size_t number_of_chords);
void json_to_chords(QList<Chord> &new_chords,
                    const nlohmann::json &json_chords);