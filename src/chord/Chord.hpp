#pragma once

#include <QString>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "interval/Interval.hpp"
#include "note/Note.hpp"
#include "rational/Rational.hpp"

struct Chord {
  std::vector<Note> notes;

  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};

[[nodiscard]] auto chords_to_json(const std::vector<Chord> &chords,
                                  size_t first_chord_number,
                                  size_t number_of_chords,
                                  bool include_notes = true) -> nlohmann::json;

void json_to_chords(std::vector<Chord> &new_chords,
                    const nlohmann::json &json_chords, size_t number_of_chords);