#pragma once

#include <QString>
#include <nlohmann/json.hpp>

#include "interval/Interval.hpp"
#include "justly/NoteColumn.hpp"
#include "rational/Rational.hpp"

struct Instrument;

template <typename T> class QList;
namespace nlohmann::json_schema {
class json_validator;
} // namespace nlohmann::json_schema

struct Note {
  const Instrument *instrument_pointer = nullptr;
  Interval interval;
  Rational beats;
  Rational velocity_ratio;
  Rational tempo_ratio;
  QString words;
};

[[nodiscard]] auto get_notes_schema() -> nlohmann::json;
[[nodiscard]] auto
get_notes_cells_validator() -> const nlohmann::json_schema::json_validator &;
[[nodiscard]] auto
notes_to_json(const QList<Note> &notes, int first_item_number,
              int number_of_notes,
              NoteColumn left_column = note_instrument_column,
              NoteColumn right_column = note_words_column) -> nlohmann::json;
void partial_json_to_notes(QList<Note> &new_items,
                           const nlohmann::json &json_notes,
                           int number_of_notes);
void json_to_notes(QList<Note> &new_items, const nlohmann::json &json_notes);