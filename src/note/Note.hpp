#pragma once

#include <QString>
#include <QtGlobal>
#include <nlohmann/json.hpp>

#include "instrument/Instrument.hpp"
#include "interval/Interval.hpp"
#include "rational/Rational.hpp"

template <typename T> class QList;
namespace nlohmann::json_schema { class json_validator; } 

struct Note {
  const Instrument *instrument_pointer = get_instrument_pointer("Marimba");
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};

[[nodiscard]] auto get_notes_schema() -> nlohmann::json;
[[nodiscard]] auto
get_notes_cells_validator() -> const nlohmann::json_schema::json_validator &;
[[nodiscard]] auto notes_to_json(const QList<Note> &notes,
                                 qsizetype first_note_number,
                                 qsizetype number_of_notes) -> nlohmann::json;
void json_to_notes(QList<Note> &new_notes, const nlohmann::json &json_notes,
                   qsizetype number_of_notes);