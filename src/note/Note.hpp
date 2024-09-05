#pragma once

#include <QString>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

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

[[nodiscard]] auto notes_to_json(const std::vector<Note> &notes,
                                 size_t first_note_number,
                                 size_t number_of_notes) -> nlohmann::json;

void json_to_notes(std::vector<Note> &new_notes,
                   const nlohmann::json &json_notes, size_t number_of_notes);